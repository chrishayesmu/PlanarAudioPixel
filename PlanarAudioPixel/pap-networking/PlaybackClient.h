#ifndef RASPBERRY_PI
#define RASPBERRY_PI

#include "NetworkStructures.h"
#include "Socket.h"
#include <queue>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZE_OF_PAYLOAD 1468
#define SIZE_OF_IP_INET_ADDRESSES 50

namespace Networking 
	{
	
	struct MESSAGEPACKET
		{
		PacketStructures::NetworkMessage messageHeader;
		char data[SIZE_OF_PAYLOAD];
		};

	Networking::ClientGUID stringToGuid( char *aBroadCastIP, char *aLocalIP );
	
	class PlaybackClient 
		{
		public:
			PlaybackClient( char *aHostName, char *aPortNumber, float aXPosition, float aYPosition );
			~PlaybackClient(){}
			void connectToServer();
			void checkInWithServer();
			int recieveMessageFromServer()
				{
				return recv ( cSocketData, &cIncomingMessage, sizeof( MESSAGEPACKET ), 0 );
				}
			
		private:
		
			int cSocketData, cSocketFlags;
			
			struct sockaddr_in cServer;
			struct hostent *cHp;
			
			char cBroadcastIP[SIZE_OF_IP_INET_ADDRESSES], cLocalIP[SIZE_OF_IP_INET_ADDRESSES];
			
			float cXPosition, cYPosition;
			
			time_t cClientReceivedPacketTimeout;
			
			MESSAGEPACKET cIncomingMessage;
		};
	}

#endif