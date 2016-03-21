#include "UdpTransport.h"
#include <iostream>
#include <sys/socket.h>
#include <errno.h>

using namespace std;

UdpTransport::UdpTransport(int udp_port)
{

	_socket = socket(AF_INET, SOCK_DGRAM, 0);
	_addrSrv.sin_addr.s_addr= htonl(INADDR_ANY);
	_addrSrv.sin_family = AF_INET;
	_addrSrv.sin_port = htons(udp_port);

	bind(_socket, (sockaddr*)&_addrSrv, sizeof(sockaddr));
	connected = false;
}

UdpTransport::~UdpTransport()
{
	close(_socket);
}

bool UdpTransport::send(char* data, unsigned int len)
{
    int slen = sendto(_socket, data, len, 0, (const sockaddr*)&_dstAddr, sizeof(_dstAddr));
	return len == slen;
}


int UdpTransport::recv(char *buff, unsigned int buf_len)
{
	struct sockaddr addr;
	int      addrLen;
	int		 recvLen;

	recvLen = recvfrom(_socket,buff, buf_len, 0, (struct sockaddr*)&addr, (socklen_t*)&addrLen);

    if (-1 == recvLen)
    {
        cout << strerror(errno) << endl;
    }
    // for debug 
    #if 0
    for (int i = 0; i < recvLen; i++)
    {
        printf("%02x ",buff[i]);
    }
    printf("\n");
    #endif
    
	if (!connected)
	{
		_dstAddr = addr;
	}
	else
	{
		if (memcmp(&addr, &_dstAddr, addrLen))
		{
			return -1;
		}
	}

	return recvLen;
}