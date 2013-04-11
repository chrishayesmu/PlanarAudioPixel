// pap-test-server.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "../pap-networking/PlaybackServer.h"
#pragma comment(lib, "../Debug/pap-networking.lib")

int _tmain(int argc, _TCHAR* argv[])
{
		WSADATA wsaData;
	WSAStartup(MAKEWORD(2,2), &wsaData);

	Networking::PlaybackServer* server;
	printf("Error code: %d\n", Networking::PlaybackServer::Create(&server));

	server->ServerStart();

	int pause = 0;
	scanf("%d", &pause);

	return 0;
}

