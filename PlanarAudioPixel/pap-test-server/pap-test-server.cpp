// pap-test-server.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "../pap-networking/PlaybackServer.h"
#pragma comment(lib, "../Debug/pap-networking.lib")

int _tmain(int argc, _TCHAR* argv[])
{

	Networking::PlaybackServer* server;
	printf("Error code: %d\n", Networking::PlaybackServer::Create(&server));

	server->ServerStart();
	server->AddTrack("C:\\Users\\Giancarlo\\Music\\Adele\\19\\12 Hometown Glory.wma", "C:\\Users\\Giancarlo\\Music\\Adele\\19\\ZuneAlbumArt.jpg");
	server->Play();

	int pause = 0;
	scanf("%d", &pause);

	return 0;
}

