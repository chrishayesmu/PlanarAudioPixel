#ifndef RASPBERRY_PI
#define RASPBERRY_PI

#include "NetworkStructures.h"
#include "Socket.h"
#include <queue>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <queue>
/*
#include <boost/thread.hpp>  
#include <boost/date_time.hpp>
#include <boost/bind.hpp>
*/

#define SIZE_OF_PAYLOAD 1468
#define SIZE_OF_IP_INET_ADDRESSES 50

namespace Networking 
	{
	struct MESSAGEPACKET
		{
		PacketStructures::NetworkMessage messageHeader;
		char data[SIZE_OF_PAYLOAD];
		};
	
	/*
	I'm overloading comparison operators for a MESSAGEPACKET queue data structure
	Using less than operator for audio samples, and greater than operator for volume samples
	Can be changed based on chris' work
	*/
	bool operator < ( MESSAGEPACKET messagePacket1, MESSAGEPACKET messagePacket2 )
		{
		return messagePacket1.messageHeader.AudioSample.SampleID > messagePacket2.messageHeader.AudioSample.SampleID;	
		}
	
	bool operator > ( MESSAGEPACKET messagePacket1, MESSAGEPACKET messagePacket2 )
		{
		return messagePacket1.messageHeader.VolumeSample.SampleID > messagePacket2.messageHeader.VolumeSample.SampleID;
		}

	Networking::ClientGUID stringToGuid( char *aBroadCastIP, char *aLocalIP );
	
	class PlaybackClient 
		{
		public:
			PlaybackClient( char *aHostName, char *aPortNumber, float aXPosition, float aYPosition );
			~PlaybackClient(){}
			void connectToServer();
			void checkInWithServer();
			
			void listenerFunction();
			void playbackFunction();
			
			int recieveMessageFromServer()
				{
				return recv ( cSocketData, &cIncomingMessage, sizeof( MESSAGEPACKET ), 0 );
				}
			int queueMessagesFromServer();
			
		private:
		
			int cSocketData, cSocketFlags;
			
			struct sockaddr_in cServer;
			struct hostent *cHp;
			
			char cBroadcastIP[SIZE_OF_IP_INET_ADDRESSES], cLocalIP[SIZE_OF_IP_INET_ADDRESSES];
			
			float cXPosition, cYPosition;
			
			time_t cClientReceivedPacketTimeout;
			
			std::priority_queue<MESSAGEPACKET, 
								std::vector<MESSAGEPACKET>, 
								std::less<std::vector<MESSAGEPACKET>::value_type> > cAudioMessageQueue;
								
			std::priority_queue<MESSAGEPACKET, 
								std::vector<MESSAGEPACKET>, 
								std::greater<std::vector<MESSAGEPACKET>::value_type> > cVolumeMessageQueue;
			
			std::queue<PacketStructures::NetworkMessage> cNetworkMessageQueue;
			
			MESSAGEPACKET cIncomingMessage;
			
			//boost::thread *cListenerThread;
		};
	}

#endif