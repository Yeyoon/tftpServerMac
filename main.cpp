#include "TftpServer.h"
using namespace std;

int main()
{
	TftpServ serv(6001);
	serv.start();
	return 0;
}