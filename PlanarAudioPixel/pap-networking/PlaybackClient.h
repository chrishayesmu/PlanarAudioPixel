#ifndef RASPBERRY_PI
#define RASPBERRY_PI

#include "NetworkStructures.h"
#include "Socket.h"
#include <queue>
#include <list>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <queue>
#include <sys/time.h>
/*
#include <boost/thread.hpp>  
#include <boost/date_time.hpp>
#include <boost/bind.hpp>
*/
#define PACKETRECEIPTTIMEOUT 2000
#define NUMBER_OF_ACCEPTABLE_EXTRA_PACKETS 5
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
		switch( messagePacket1.messageHeader.ControlByte )
			{
			case ControlBytes::SENDING_AUDIO:
				if( messagePacket1.messageHeader.AudioSample.TrackID != messagePacket2.messageHeader.AudioSample.TrackID )
					{
					return messagePacket1.messageHeader.AudioSample.TrackID > messagePacket2.messageHeader.AudioSample.TrackID;
					}
				else
					{
					return messagePacket1.messageHeader.AudioSample.SampleID > messagePacket2.messageHeader.AudioSample.SampleID;
					}
			default:
				if( messagePacket1.messageHeader.VolumeSample.TrackID != messagePacket2.messageHeader.VolumeSample.TrackID )
					{
					return messagePacket1.messageHeader.VolumeSample.TrackID > messagePacket2.messageHeader.VolumeSample.TrackID;
					}
				else
					{
					return messagePacket1.messageHeader.VolumeSample.SampleID > messagePacket2.messageHeader.VolumeSample.SampleID;
					}
			}
		}
	
	bool operator > ( MESSAGEPACKET messagePacket1, MESSAGEPACKET messagePacket2 )
		{
		switch( messagePacket1.messageHeader.ControlByte )
			{
			case ControlBytes::SENDING_AUDIO:
				if( messagePacket1.messageHeader.AudioSample.TrackID != messagePacket2.messageHeader.AudioSample.TrackID )
					{
					return messagePacket1.messageHeader.AudioSample.TrackID < messagePacket2.messageHeader.AudioSample.TrackID;
					}
				else
					{
					return messagePacket1.messageHeader.AudioSample.SampleID < messagePacket2.messageHeader.AudioSample.SampleID;
					}
			default:
				if( messagePacket1.messageHeader.VolumeSample.TrackID != messagePacket2.messageHeader.VolumeSample.TrackID )
					{
					return messagePacket1.messageHeader.VolumeSample.TrackID < messagePacket2.messageHeader.VolumeSample.TrackID;
					}
				else
					{
					return messagePacket1.messageHeader.VolumeSample.SampleID < messagePacket2.messageHeader.VolumeSample.SampleID;
					}
			}
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
			int checkForDroppedPacketsAndAddPacketToList( MESSAGEPACKET aMessagePacket );
			
		private:
		
			int cSocketData, cSocketFlags;
			
			struct sockaddr_in cServer;
			struct hostent *cHp;
			
			char cBroadcastIP[SIZE_OF_IP_INET_ADDRESSES], cLocalIP[SIZE_OF_IP_INET_ADDRESSES];
			
			float cXPosition, cYPosition;
			
			std::list<MESSAGEPACKET> cAudioMessageList, cAudioMessageListExtraPackets;
			std::list<MESSAGEPACKET> cVolumeMessageList, cVolumeMessageListExtraPackets;
			std::queue<PacketStructures::NetworkMessage> cNetworkMessageQueue;
			
			
			
			MESSAGEPACKET cIncomingMessage;
			
			//boost::thread *cListenerThread;
		};
	}

#endif