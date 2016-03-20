#include "TftpServer.h"
using namespace std;

int main()
{
	TftpServ serv(69);
	serv.start();
	return 0;
}