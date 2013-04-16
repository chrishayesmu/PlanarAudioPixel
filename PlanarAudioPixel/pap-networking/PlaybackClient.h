#ifdef PLAYBACKCLIENT

#include "../pap-file-io/Logger.h"
#include "NetworkStructures.h"
#include "Socket.h"
#include <fcntl.h>
#include <queue>
#include <list>
#include <iostream>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <queue>
#include <sys/time.h>

#include <boost/thread.hpp>  
#include <boost/date_time.hpp>
#include <boost/bind.hpp>

#define AUDIOHEADERRANGE 1
#define NUMBER_OF_ACCEPTABLE_EXTRA_PACKETS 5
#define PACKETRECEIPTTIMEOUT 2000
#define SIZE_OF_PAYLOAD 1468
#define SIZE_OF_IP_INET_ADDRESSES 50

namespace Networking 
	{
	struct MESSAGEPACKET
		{
		PacketStructures::NetworkMessage messageHeader;
		char data[SIZE_OF_PAYLOAD];
		};
	
	
	bool operator < ( MESSAGEPACKET messagePacket1, MESSAGEPACKET messagePacket2 );
	bool operator > ( MESSAGEPACKET messagePacket1, MESSAGEPACKET messagePacket2 );

	class PlaybackClient 
		{
		public:
			PlaybackClient( char *aHostName, char *aPortNumber, float aXPosition, float aYPosition, char *aIPAddress = NULL );
			~PlaybackClient(){}
			
			void listenerFunction();
			void changePlaybackStatus(  const unsigned char aControlByte );
			void playbackFunction();
			
			int recieveMessageFromServer( int aSize = sizeof( PacketStructures::NetworkMessage ), void *aAddressPtr = NULL )
				{
				return recv ( cSocketData, ( aAddressPtr == NULL ) ? &cIncomingMessage : aAddressPtr, aSize, 0 );
				}
			
			int sendMessageToServer( const unsigned char aControlByte = ControlBytes::SYNCHRONIZATION_REQUEST, 
									Networking::trackid_t aTrackID = 0,
									Networking::sampleid_t aSampleID = 0,
									Networking::sampleid_t aBufferRangeStartID = 0,
									Networking::sampleid_t aBufferRangeEndID = 0,
									Networking::requestid_t aRequestID = 0 );
			
			int queueMessagesFromServer();
			int AddAudioOrVolumePacketToList( MESSAGEPACKET aMessagePacket );
			void checkForMissingAudioHeader( MESSAGEPACKET aMessagePacket );
			
		private:
		
			int cSocketData, cSocketFlags;
			
			struct sockaddr_in cServer;
			struct hostent *cHostEntry;
			
			char cBroadcastIP[SIZE_OF_IP_INET_ADDRESSES], cLocalIP[SIZE_OF_IP_INET_ADDRESSES];
			
			float cXPosition, cYPosition;
			
			std::list<MESSAGEPACKET> cAudioMessageList, cAudioMessageListExtraPackets;
			std::list<MESSAGEPACKET> cVolumeMessageList, cVolumeMessageListExtraPackets;
			std::queue<PacketStructures::NetworkMessage> cNetworkMessageQueue;
			
			bool cPlay, cPause;
			boost::mutex cPlaybackLock;
			
			MESSAGEPACKET cIncomingMessage;
			
			boost::thread *cListenerThread, *cPlaybackThread;
			
			bool cFirstAudioPacket, cIsAudioCaughtUp;
			int cAudioPacketCounter, cVolumePacketCounter;
			timeval cAudioTimeoutStartTime, cVolumeTimeoutStartTime, cCurrentTime;
			Networking::trackid_t cNextAudioTrackID, cNextVolumeTrackID;
			Networking::sampleid_t cNextAudioSampleID, cNextAudioBufferRangeStartID, cNextAudioBufferRangeEndID;
			Networking::sampleid_t cNextVolumeSampleID, cNextVolumeBufferRangeStartID, cNextVolumeBufferRangeEndID;
		};
	}

#endif