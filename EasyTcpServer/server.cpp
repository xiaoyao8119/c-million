#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include<windows.h>
#include<WinSock2.h>
#include<stdio.h>
#include<vector>
#pragma comment(lib,"ws2_32.lib")

enum CMD
{
	CMD_LOGIN,
	CMD_LOGIN_RESULT,
	CMD_LOGOUT,
	CMD_LOGOUT_RESULT,
	CMD_NEW_USER_JOIN,
	CMD_ERROR
};
struct DataHeader
{
	short dataLength;
	short cmd;
};
//DataPackage
struct Login: public DataHeader
{
	Login()
	{
		dataLength = sizeof(Login);
		cmd = CMD_LOGIN;
	}
	char userName[32];
	char PassWord[32];
};

struct LoginResult : public DataHeader
{
	LoginResult()
	{
		dataLength = sizeof(LoginResult);
		cmd = CMD_LOGIN_RESULT;
		result = 0;
	}
	int result;
};
struct NewUserJoin :public DataHeader
{
	NewUserJoin() {
		dataLength = sizeof(NewUserJoin);
		cmd = CMD_NEW_USER_JOIN;
		scok = 0;
	 }
	int scok;
};

struct Logout : public DataHeader
{
	Logout()
	{
		dataLength = sizeof(Logout);
		cmd = CMD_LOGOUT;
	}
	char userName[32];
};

struct LogoutResult : public DataHeader
{
	LogoutResult()
	{
		dataLength = sizeof(LogoutResult);
		cmd = CMD_LOGIN_RESULT;
		result = 0;
	}
	int result;
};
std::vector<SOCKET> g_clients;
int processor(SOCKET _cSock) {
	DataHeader header = {};
	// 5 接收客户端数据
	int nLen = recv(_cSock, (char*)& header, sizeof(DataHeader), 0);
	if (nLen <= 0)
	{
		printf("客户端已退出，任务结束。");
		return -1;
	}
	switch (header.cmd)
	{
	case CMD_LOGIN:
	{
		Login login = {};
		recv(_cSock, (char*)& login + sizeof(DataHeader), sizeof(Login) - sizeof(DataHeader), 0);
		printf("收到命令：CMD_LOGIN,数据长度：%d,userName=%s PassWord=%s\n", login.dataLength, login.userName, login.PassWord);
		//忽略判断用户密码是否正确的过程
		LoginResult ret;
		send(_cSock, (char*)& ret, sizeof(LoginResult), 0);
	}
	break;
	case CMD_LOGOUT:
	{
		Logout logout = {};
		recv(_cSock, (char*)& logout + sizeof(DataHeader), sizeof(logout) - sizeof(DataHeader), 0);
		printf("收到命令：CMD_LOGOUT,数据长度：%d,userName=%s \n", logout.dataLength, logout.userName);
		//忽略判断用户密码是否正确的过程
		LogoutResult ret;
		send(_cSock, (char*)& ret, sizeof(ret), 0);
	}
	break;
	default:
		header.cmd = CMD_ERROR;
		header.dataLength = 0;
		send(_cSock, (char*)& header, sizeof(header), 0);
		break;
	}

}


int main()
{
	//启动Windows socket 2.x环境
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver, &dat);
	//------------

	//-- 用Socket API建立简易TCP服务端
	// 1 建立一个socket 套接字
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //AF_INET 为IPV4，，SOCK_STREAM面向流， IPPROTO_TCP  TCP协议
	// 2 bind 绑定用于接受客户端连接的网络端口
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(4567);//host to net unsigned short
	_sin.sin_addr.S_un.S_addr = INADDR_ANY; //全部的IP地址都可以访问  //inet_addr("127.0.0.1");
	if (SOCKET_ERROR == bind(_sock, (sockaddr*)&_sin, sizeof(_sin)))    // (sockaddr*)强制转换
	{
		printf("错误,绑定网络端口失败...\n");
	}
	else {
		printf("绑定网络端口成功...\n");
	}
	// 3 listen 监听网络端口
	if (SOCKET_ERROR == listen(_sock, 5))// 最大等待5个人同时连接
	{
		printf("错误,监听网络端口失败...\n");
	}
	else {
		printf("监听网络端口成功...\n");
	}
	/*// 4 accept 等待接受客户端连接
	sockaddr_in clientAddr = {};
	int nAddrLen = sizeof(sockaddr_in);
	SOCKET _cSock = INVALID_SOCKET;
	

	_cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
	if (INVALID_SOCKET == _cSock)
	{
		printf("错误,接受到无效客户端SOCKET...\n");
	}
	printf("新客户端加入：socket = %d,IP = %s \n", (int)_cSock, inet_ntoa(clientAddr.sin_addr));
	// char msgBuf[]="hello,i am server";  send(_cSock,msgBuf,strlen(msgBuf)+1,0); */

	while (true)
	{    
		fd_set fdRead;
		fd_set fdWrite;
		fd_set fdExp;
		FD_ZERO(&fdRead);
		FD_ZERO(&fdWrite);
		FD_ZERO(&fdExp);
		FD_SET(_sock, &fdRead);
		FD_SET(_sock, &fdWrite);
		FD_SET(_sock, &fdExp);
		for (int n = (int)g_clients.size()-1; n >=0; n--) {
			FD_SET(g_clients[n], &fdRead);
		}
		timeval t = { 1,0 };
		int ret=select(_sock + 1, &fdRead, &fdWrite, &fdExp, &t);     //NULL 换成t
		if (ret < 0) {
			printf("select任务结束。\n");
			break;
		}
		if (FD_ISSET(_sock, &fdRead)) {
			FD_CLR(_sock, &fdRead);
			// 4 accept 等待接受客户端连接
			sockaddr_in clientAddr = {};
			int nAddrLen = sizeof(sockaddr_in);
			SOCKET _cSock = INVALID_SOCKET;


			_cSock = accept(_sock, (sockaddr*)& clientAddr, &nAddrLen);
			if (INVALID_SOCKET == _cSock)
			{
				printf("错误,接受到无效客户端SOCKET...\n");
			}
			for (int n = (int)g_clients.size() - 1; n >= 0; n--) {
				NewUserJoin userjoin;
				send(g_clients[n], (const char*)&userjoin, sizeof(NewUserJoin), 0);
			}
			g_clients.push_back(_cSock);
			printf("新客户端加入：socket = %d,IP = %s \n", (int)_cSock, inet_ntoa(clientAddr.sin_addr));		
		}		
		for (int n =0; n<(int)fdRead.fd_count; n++) {
			if (-1 == processor(fdRead.fd_array[n])) {
				auto iter = find(g_clients.begin(), g_clients.end(), fdRead.fd_array[n]);
				if (iter != g_clients.end()) {
					g_clients.erase(iter);
				}
			} 
		}
		//printf("处理其他业务\n");
	}
	for (int n = (int)g_clients.size() - 1; n >= 0; n--) {
		closesocket(g_clients[n]);
	}
	// 8 关闭套节字closesocket
	closesocket(_sock);
	//------------
	//清除Windows socket环境
	WSACleanup();
	printf("已退出。");
	getchar();
	return 0;
}