#ifndef _UDP_TRANSPORT_H
#define _UDP_TRANSPORT_H

#include "Transport.h"
#include <sys/socket.h>
#include <netinet/in.h>

class UdpTransport : public Transport {
public:
	UdpTransport(int udp_port);
	~UdpTransport();
	bool send(char* data, unsigned int len);
	int recv(char* buff, unsigned int buf_len);

private:
	int _socket;
	struct sockaddr_in _addrSrv;
	struct sockaddr _dstAddr;
	int      _dstAddrLen;
	bool    connected;
};
#endif /*_UDP_TRANSPORT_H*/
