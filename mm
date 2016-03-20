bin = tftpServer
src = ./main.cpp ./TftpServer.cpp ./UdpTransport.cpp 
obj = ./main.o ./TftpServer.o ./UdpTransport.o 
head = ./UdpTransport.h ./TftpServer.h ./Transport.h

$(bin):$(src) $(head)
	g++  -g $(src) -o $@    
clean:
	rm -fr *.o
	rm -fr $(bin)
