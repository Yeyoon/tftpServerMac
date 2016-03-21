#include "TftpServer.h"
#include <arpa/inet.h>
#include <fstream>
#include <iostream>

using namespace std;

TftpServ::TftpServ(unsigned short port)
{
	m_udpPort = port;
}

bool TftpServ::handleAck(ACK* ack)
{
	if (ack->block < m_blockNumber)
	{
		cout << "retransf last data" << endl;
		return m_buff_len == sendBlockData(m_buff_len);
	}
	else if (ack->block == m_blockNumber)
	{
		m_buff_len = fillBufffromFile();
		if (m_buff_len > 0)
		{
			return m_buff_len == sendBlockData(m_buff_len);
		}
		return false;
	}

	return false;
}

bool TftpServ::handleRrq(RRQ* rrq)
{
	char* filename = rrq->filename;
	if (m_file)
	{
		sendError(TFTPERR_NOT_DEFINED,"duplicate read request");
		return false;
	}

	m_file = fopen(filename, "r");
	if (!m_file)
	{
		sendError(TFTPERR_ACCESS_VIOLATION,"file open error");
		return false;
	}

    m_blockNumber = 0;
	m_buff_len = fillBufffromFile();

	if (m_buff_len > 0)
	{
		return m_buff_len == sendBlockData(m_buff_len);
	}

	return false;
}

int TftpServ::fillBufffromFile()
{
	if (m_file)
	{
		return fread(m_buff, 1, TFTP_DATA_LEN, m_file);
	}

	return -1;
}

int TftpServ::sendBlockData(int len)
{
	m_blockNumber++;
	tftpPacket* packet;
	memmove(m_buff + 4, m_buff, len);
	packet = (tftpPacket*)m_buff;
	packet->opCode = htons((unsigned short)TFTPOPCODE_DATA);
    DATA* data = (DATA*)((char*)packet + sizeof(packet->opCode));
    data->block = htons(m_blockNumber);

    // for debug
    #if 0
    for (int i = 0; i < len + 4; i++)
    {
        printf("%02x ",m_buff[i]);
    }
    printf("\n");
    #endif
    
	if (m_pTransport->send(m_buff, len+4))	  // total len is datalen + 4
	{
		return len;
	}

	return -1;
}

bool TftpServ::handleWrq(WRQ* wrq)
{
	if (m_file)
	{
		sendError(TFTPERR_FILE_ALREADY_EXISTS,"duplicate write request");
		return false;
	}

	m_file = fopen(wrq->filename, "w");
	if (!m_file)
	{
		sendError(TFTPERR_ACCESS_VIOLATION,"file open for write error");
		return false;
	}

	//return 4 == sendAck();
    m_blockNumber = 0;
    sendAck();
    return true;
}

int TftpServ::sendError(tftpErrorCode_e e, char* errMsg)
{
	tftpPacket* p = (tftpPacket*)m_buff;
	p->opCode = TFTPOPCODE_ERR;
    _ERROR* err = (_ERROR*)((char*)p + sizeof(p->opCode));
	strncpy(err->errMsg, errMsg, strlen(errMsg));
	err->errMsg[strlen(errMsg)] = 0;
	err->errorCode = htons((unsigned short)e);

	m_pTransport->send(m_buff, strlen(errMsg) + 5);
}

int TftpServ::sendAck()
{
	tftpPacket* packet = (tftpPacket*)m_buff;
	packet->opCode = htons((unsigned short)TFTPOPCODE_ACK);
    ACK* ack = (ACK*)(packet->packetContent);
	ack->block = htons(m_blockNumber);
	m_blockNumber++;

	bool r = m_pTransport->send(m_buff, 4);
    if (true == r)
    {
        return 4;
    }
    else
    {
        return -1;
    } // ack len is 4
}

bool TftpServ::handleData(DATA* data, int dataLen)
{
	// this only happens in wrq
	if (m_file && data->block == m_blockNumber)
	{
		fwrite(data->data, dataLen, 1, m_file);

		if (dataLen < TFTP_DATA_LEN)
		{
			// this is the terminal packet
			sendAck();
			stop();
		}
		return true;
	}

	return false;
}

bool TftpServ::handleError(_ERROR* err)
{
	cout << "errCode : " << err->errorCode << "\terrMsg : " << err->errMsg << endl;
	stop();
	return false;
}

void TftpServ::handlePacket(tftpPacket* packet, int packetLen)
{
	tftpOpcode_e op = (tftpOpcode_e)ntohs((packet->opCode));
	bool ret = true;
    
	switch (op)
	{
	case TFTPOPCODE_ACK:
    {
        ACK* ack = (ACK*)(packet->packetContent);
		ret = handleAck(ack);
		break;
    }

	case TFTPOPCODE_DATA:
    {
		int dataLen = packetLen - sizeof(unsigned short) - sizeof(unsigned short); // opcode len + blockNumber len
        DATA data;
        data.block = ntohs(*(unsigned short*)(packet->packetContent));
        data.data = (char*)(packet->packetContent) + 2;
		ret = handleData(&data, dataLen);
		break;
    }

	case TFTPOPCODE_RRQ:
    {
        RRQ rrq;
        rrq.filename = (char*)(packet->packetContent);
        rrq.mode = (char*)rrq.filename + strlen(rrq.filename) + 1;
		ret = handleRrq(&rrq);
		break;
    }

	case TFTPOPCODE_WRQ:
    {
        WRQ wrq;
        wrq.filename = (char*)packet->packetContent;
        wrq.mode = (char*)wrq.filename +  strlen(wrq.filename) + 1;
		ret = handleWrq(&wrq);
		break;
    }

	case TFTPOPCODE_ERR:
    {
        _ERROR* err = (_ERROR*)(packet->packetContent);
        err->errorCode = *((unsigned short*)(packet->packetContent));
        err->errorCode = ntohs(err->errorCode);
        err->errMsg = (char*)(packet->packetContent) + sizeof(err->errorCode);
		ret = handleError(err);
		break;
    }

	default:
		cout << "WRONG TFTP Operation : " << op << endl;
		break;
	}

	if (!ret)
	{
		stop();
	}
}

void TftpServ::start()
{
	m_pTransport = new UdpTransport(m_udpPort);
	m_tftpServStatus = TFTPSERV_START;
	
	while (m_tftpServStatus == TFTPSERV_START)
	{
		int recvLen = m_pTransport->recv(m_buff, TFTP_BUFF_LEN);
		if (recvLen <= 0)
		{
			// something wrong just ignore this time
			continue;
		}

		handlePacket((tftpPacket*)m_buff, recvLen);
	}
}

void TftpServ::stop()
{
	m_tftpServStatus = TFTPSERV_STOP;

	if (m_file)
	{
		fclose(m_file);
		m_file = NULL;
	}
}

