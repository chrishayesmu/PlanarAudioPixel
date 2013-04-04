#ifdef RASPBERRY_PI

#include "PlaybackClient.h"
#include "Socket.h"
#include "NetworkGlobals.h"
#include "ControlByteConstants.h"
#include "NetworkConstants.h"

/******************************Needs Work******************************/

namespace Networking 
	{
	/*
	This function converts IP address strings into the format of a ClientGUID
	There is no error checking.
	*/
	Networking::ClientGUID stringToGuid( char *aBroadCastIP, char *aLocalIP ) 
		{
		Networking::ClientGUID guid;
		sscanf( aBroadCastIP, "%uh.%uh.%uh.%uh", &guid.BroadcastIP.Byte1, &guid.BroadcastIP.Byte2, &guid.BroadcastIP.Byte3, &guid.BroadcastIP.Byte4 );
		sscanf( aLocalIP, "%uh.%uh.%uh.%uh", &guid.LocalIP.Byte1, &guid.LocalIP.Byte2, &guid.LocalIP.Byte3, &guid.LocalIP.Byte4 );
		
		return guid;
		}
		
	/*
	PlaybackClient constructor just sets up udp stuff
	*/
	PlaybackClient::PlaybackClient( char *aHostName, char *aPortNumber, float aXPosition, float aYPosition, char *aIPAddress )
		: 	cXPosition( aXPosition ),
			cYPosition( aYPosition ),
			cPlay( false ),
			cPause( false ),
			cAudioPacketCounter( 0 ),
			cVolumePacketCounter( 0 ),
			cNextAudioTrackID( -1 ),
			cNextVolumeTrackID( -1 ),
			cNextAudioSampleID( -1 ),
			cNextAudioBufferRangeStartID( -1 ),
			cNextAudioBufferRangeEndID( -1 ),
			cNextVolumeSampleID( -1 ),
			cNextVolumeBufferRangeStartID( -1 ),
			cNextVolumeBufferRangeEndID( -1 )			
		{
		timeval mTimeval;
		gettimeofday(&mTimeval, 0);
		time_t curtime = mTimeval.tv_sec;
		tm *mTimePtr = localtime(&curtime);
		char mTempBuffer[100];
		sprintf( mTempBuffer, "LogFileClient:%02d:%02d:%02d:%03d", mTimePtr->tm_hour, mTimePtr->tm_min, mTimePtr->tm_sec, mTimeval.tv_usec );
		Logger::openLogFile( mTempBuffer, true );
		Logger::logNotice( "Starting playback client" );
		Logger::logNotice( "Startup Time in seconds: %d", mTimeval.tv_sec );
		
		cSocketData = socket( AF_INET, SOCK_DGRAM, 0 );
		fcntl( cSocketData, F_SETFL, O_NONBLOCK );
		cServer.sin_family = AF_INET;
		
		/*
		Get local machine info
		*/
		/******************************Needs Work******************************/
		//Need to figure out how to get a non local ip address
		gethostname( cBroadcastIP, SIZE_OF_IP_INET_ADDRESSES );
		Logger::logNotice( "----UDPClient1 running at host NAME: %s", cBroadcastIP );
		cHostEntry = gethostbyname( cBroadcastIP );
		memcpy( &( cServer.sin_addr ), cHostEntry->h_addr, cHostEntry->h_length );
		Logger::logNotice( "(UDPClient1 INET ADDRESS is: %s )", inet_ntoa( cServer.sin_addr ) );
		strncpy( cLocalIP, inet_ntoa( cServer.sin_addr ), SIZE_OF_IP_INET_ADDRESSES );
		//Temporary until I find a way to get non local ip address
		strncpy( cBroadcastIP, aIPAddress, SIZE_OF_IP_INET_ADDRESSES );
		Logger::logNotice( "Client IP Address: %s", cBroadcastIP );

		/*
		Segmentation fault will occur with bad aHostName
		*/
		cHostEntry = gethostbyname( aHostName );
		memcpy( &(cServer.sin_addr.s_addr), cHostEntry->h_addr, cHostEntry->h_length);
		cServer.sin_port = htons( atoi( aPortNumber ) );
		
		
		connectToServer();
		cListenerThread = new boost::thread( boost::bind ( &Networking::PlaybackClient::listenerFunction, this ) );
		//cPlaybackThread = new boost::thread( boost::bind ( &Networking::PlaybackClient::playbackFunction, this ) );
		playbackFunction();
		}
		
	/*
	1)This function broadcasts a new connection message
	2)After broadcasting the message, it waits to recieve a response from the server.
	3)If it doesn't receive a response from the server before the timeout, it retransmits
	4)If it receives a response, but not a new connection response, it retransmits
	5)It repeats this process until it recieves the correct message from the server
	*/
	void PlaybackClient::connectToServer() 
		{
		int mRecieve = -1;
		timeval mTimeoutTime, cCurrentTime;
		Logger::logNotice( "Connecting to server" );
		
		do
			{
			Logger::logNotice( "Sending NEW_CONNECTION packet" );
			sendMessageToServer( ControlBytes::NEW_CONNECTION );
			
			gettimeofday( &mTimeoutTime, 0 );
			gettimeofday( &cCurrentTime, 0 );
			
			while( ( cCurrentTime.tv_sec - mTimeoutTime.tv_sec ) * 1000 + ( cCurrentTime.tv_usec - mTimeoutTime.tv_usec ) / 1000 
					< PACKETRECEIPTTIMEOUT )
				{
				if( ( mRecieve = recieveMessageFromServer() ) != -1 )
					{
					break;
					}
				
				gettimeofday( &cCurrentTime, 0 );
				}
				
			if( mRecieve != -1 )
				{
				if( cIncomingMessage.messageHeader.ControlByte == ControlBytes::NEW_CONNECTION )
					{
					Logger::logNotice( "Received NEW_CONNECTION packet" );
					Logger::logNotice( "Successfully connected to server" );
					return;
					}
				}
			
			Logger::logWarning( "Server response to NEW_CONNECTION timed out." );
			}while( 1 );		
		}
	
	void PlaybackClient::listenerFunction()
		{
		int mNewMessages;
		timeval mCheckInTime, cCurrentTime;
		gettimeofday( &mCheckInTime, 0 );
				
		do
			{
			/*
			All of the Timeout stuff together.
			I probably don't need to keep calling gettimeofday() as it happens so close together, but as we're
				aiming for time precision, I did it nonetheless.
			*/
			
			gettimeofday( &cCurrentTime, 0 );
			if( ( cCurrentTime.tv_sec - mCheckInTime.tv_sec ) * 1000 + ( cCurrentTime.tv_usec - mCheckInTime.tv_usec ) / 1000 
					> CLIENT_CHECKIN_DELAY )
				{
				Logger::logNotice( "Initiating periodic check in with server" );
				checkInWithServer();
				Logger::logNotice( "PERIODIC_CHECK_IN packet sent" );
				gettimeofday( &mCheckInTime, 0 );
				}
			
			gettimeofday( &cCurrentTime, 0 );
			if( ( cCurrentTime.tv_sec - cAudioTimeoutStartTime.tv_sec ) * 1000 
			      + ( cCurrentTime.tv_usec - cAudioTimeoutStartTime.tv_usec ) / 1000 
				  > PACKETRECEIPTTIMEOUT && cNextAudioTrackID != -1 )
				{
				int mTimeOut = ( cCurrentTime.tv_sec - cAudioTimeoutStartTime.tv_sec ) * 1000 
								 + ( cCurrentTime.tv_usec - cAudioTimeoutStartTime.tv_usec ) / 1000;
								 
				sendMessageToServer( ControlBytes::RESEND_AUDIO, cNextAudioTrackID, cNextAudioSampleID,
									 cNextAudioBufferRangeStartID, cNextAudioBufferRangeEndID );
				
				Logger::logWarning( "---AUDIOPACKET TIMEOUT---" );
				Logger::logNotice( "Timeout: %d - AudioPacketCounter: %d", mTimeOut, cAudioPacketCounter );
				Logger::logNotice( "Sent a RESEND_AUDIO packet to server." );
				Logger::logWarning( "Resetting timeout" );
				
				gettimeofday( &cAudioTimeoutStartTime, 0 );
				}
				
			gettimeofday( &cCurrentTime, 0 );
			if( ( cCurrentTime.tv_sec - cVolumeTimeoutStartTime.tv_sec ) * 1000 
			      + ( cCurrentTime.tv_usec - cVolumeTimeoutStartTime.tv_usec ) / 1000 
				  > PACKETRECEIPTTIMEOUT && cNextVolumeTrackID != -1 )
				{
				int mTimeOut = ( cCurrentTime.tv_sec - cVolumeTimeoutStartTime.tv_sec ) * 1000 
								 + ( cCurrentTime.tv_usec - cVolumeTimeoutStartTime.tv_usec ) / 1000;
								 
				sendMessageToServer( ControlBytes::RESEND_VOLUME, cNextVolumeTrackID, cNextVolumeSampleID,
									 cNextVolumeBufferRangeStartID, cNextVolumeBufferRangeEndID );
				
				Logger::logWarning( "---VOLUMEPACKET TIMEOUT---" );
				Logger::logNotice( "Timeout: %d - VolumePacketCounter: %d", mTimeOut, cVolumePacketCounter );
				Logger::logNotice( "Sent a RESEND_VOLUME packet to server." );
				Logger::logWarning( "Resetting timeout" );
				
				gettimeofday( &cVolumeTimeoutStartTime, 0 );
				}
			
			mNewMessages = queueMessagesFromServer();
			if( mNewMessages != 0 )
				{
				Logger::logNotice( "Added %d new messages to message queues!", mNewMessages );
				}
			}while( 1 );
		}
	
	/*
	Changes class state variables cPlay and Cpause based on the type of control byte passed in
	*/
	void PlaybackClient::changePlaybackStatus(  const unsigned char aControlByte )
		{
		switch( aControlByte )
			{
			case ControlBytes::BEGIN_PLAYBACK:
					{
					boost::mutex::scoped_lock lock( cPlaybackLock );
					cPlay = true;
					Logger::logNotice( "Started audio playback."  );
					}
				break;
			case ControlBytes::STOP_PLAYBACK:
					{
					boost::mutex::scoped_lock lock( cPlaybackLock );
					cPlay = false;
					Logger::logNotice( "Stopped audio playback."  );
					}
				break;
			case ControlBytes::PAUSE_PLAYBACK:
					{
					boost::mutex::scoped_lock lock( cPlaybackLock );
					cPause = ( cPause ) ? false : true;
					( cPause ) ? Logger::logNotice( "Paused audio playback."  ) : Logger::logNotice( "Unpaused audio playback."  );
					}
				break;
			}
		}
		
	void PlaybackClient::playbackFunction()
		{
		do
			{
			/******************************Needs Work******************************/
			//Waiting for Chris' stuff
			//Right now this logic would have raped the server with audio/volume resend requests or broke my logic, probably the latter
			//Need to fine tune my code to handle emptied queues, for now I'm taking it out. Left the comments for convenient testing later
			if( !cAudioMessageList.empty() )
				{
				Logger::logNotice( "There are some Audio messages, I should probably do something about those."  );
				//cAudioMessageList.pop_front();
				}
			
			if( !cVolumeMessageList.empty() )
				{
				Logger::logNotice( "There are some Volume messages, I should probably do something about those."  );
				//cVolumeMessageList.pop_front();
				}
			
			if( !cNetworkMessageQueue.empty() )
				{
				Logger::logNotice( "Handled a Network message."  );
				
				cNetworkMessageQueue.pop();
				}
			sleep( 1 ); //only in here for testing purposes so we don't blow up logfiles
			}while( 1 );
		}
	
	/*
	This function sends a check in message to the server. Nothing else.
	*/
	void PlaybackClient::checkInWithServer()
		{
		sendMessageToServer( ControlBytes::PERIODIC_CHECK_IN );
		}
	
	/*
	Creates and sends messages to servers.
	*/
	int PlaybackClient::sendMessageToServer( const unsigned char aControlByte, 
											Networking::trackid_t aTrackID,
											Networking::sampleid_t aSampleID,
											Networking::sampleid_t aBufferRangeStartID,
											Networking::sampleid_t aBufferRangeEndID,
											Networking::requestid_t aRequestID )
		{
		PacketStructures::NetworkMessage mMessage;
		mMessage.ControlByte = aControlByte;
		
		switch( aControlByte )
			{
			case ControlBytes::RESEND_AUDIO:
			case ControlBytes::RESEND_VOLUME:
				//Same data types for audio resend and volume resend, so handle both in same statement
				mMessage.AudioResendRequest.TrackID = aTrackID;
				mMessage.AudioResendRequest.SampleID = aSampleID;
				mMessage.AudioResendRequest.BufferRangeStartID = aBufferRangeStartID;
				mMessage.AudioResendRequest.BufferRangeEndID = aBufferRangeEndID;
				
				Logger::logNotice( "Created a RESEND packet." );
				break;
				
			case ControlBytes::BEGIN_PLAYBACK:
			case ControlBytes::STOP_PLAYBACK:
			case ControlBytes::PAUSE_PLAYBACK:
				/*
				I didn't see and acknowledgment control byte, so I assume you reseond with the same type of byte you
				are acknowledging.
				*/
				mMessage.TransportControl.clientID = stringToGuid( cBroadcastIP, cLocalIP );
				mMessage.TransportControl.requestID = aRequestID;
				
				Logger::logNotice( "Created a TransportControl packet." );
				break;
				
			default:
				/*
				All other client to server messages have the following data types
				*/
				mMessage.ClientCheckIn.clientID = stringToGuid( cBroadcastIP, cLocalIP );
				mMessage.ClientCheckIn.position.x = cXPosition;
				mMessage.ClientCheckIn.position.y = cYPosition;
				
				Logger::logNotice( "Created a network message packet." );
			}
		
		Logger::logNotice( "Sending packet to server." );
		return sendto( cSocketData, &mMessage, sizeof( PacketStructures::NetworkMessage ), 0, 
						(const sockaddr*)&cServer, sizeof( cServer ) );
		}
	
	/*
	This function checks for messages from the server, parses the messages, and places the messages in the appropriate queue.
	It returns the number of messages it placed in the queues.
	*/
	int PlaybackClient::queueMessagesFromServer()
		{
		static MESSAGEPACKET *mPlaceHolder, mTempNetworkPacket;
		static PacketStructures::NetworkMessage mTempNetworkMessage;
		
		//No messages
		if( recieveMessageFromServer() == -1 )
			{
			return 0;
			}
				
		
		mPlaceHolder = &cIncomingMessage;
		
		switch( mPlaceHolder->messageHeader.ControlByte )
			{
			case ControlBytes::DISCONNECT:
				Logger::logNotice( "Recieved ControlBytes::DISCONNECT packet from server. Exiting playback client." );
				Logger::closeLogFile();
				exit( ControlBytes::DISCONNECT );
				
			case ControlBytes::SENDING_AUDIO:
			case ControlBytes::SENDING_VOLUME:
				Logger::logNotice( "Received an Audio/Volume packet." );
				
				recieveMessageFromServer( mPlaceHolder->messageHeader.Extra._dataLength, mPlaceHolder->data );
				memcpy( &mTempNetworkPacket, mPlaceHolder, 
						sizeof( PacketStructures::NetworkMessage ) + mPlaceHolder->messageHeader.Extra._dataLength );
				checkForDroppedPacketsAndAddPacketToList( mTempNetworkPacket );
				break;
			
			case ControlBytes::BEGIN_PLAYBACK:
			case ControlBytes::STOP_PLAYBACK:
			case ControlBytes::PAUSE_PLAYBACK:
				Logger::logNotice( "Received a TransportControl packet." );
				
				changePlaybackStatus( mPlaceHolder->messageHeader.ControlByte );
				sendMessageToServer( mPlaceHolder->messageHeader.ControlByte, 
									0, 0, 0, 0, //placeholders to get to last field
									mPlaceHolder->messageHeader.TransportControl.requestID );
				break;
				
			default:
				Logger::logNotice( "Received a network message packet." );
				
				memcpy( &mTempNetworkMessage, mPlaceHolder, sizeof( PacketStructures::NetworkMessage ) );
				cNetworkMessageQueue.push( mTempNetworkMessage );
			}
		
		return 1;
		}
		
	int PlaybackClient::checkForDroppedPacketsAndAddPacketToList( MESSAGEPACKET aMessagePacket )
		{
		switch( aMessagePacket.messageHeader.ControlByte )
			{
			case ControlBytes::SENDING_AUDIO:
				if( cNextAudioTrackID == -1 )
					{
					Logger::logNotice( "Successfully added an audio packet to queue when queue WAS empty" );
					Logger::logNotice( "SampleID: %d - cNextAudioSampleID: %d", aMessagePacket.messageHeader.AudioSample.SampleID, cNextAudioSampleID );
					Logger::logNotice( "TrackID: %d - cNextAudioTrackID: %d", aMessagePacket.messageHeader.AudioSample.TrackID, cNextAudioTrackID );
					
					cAudioPacketCounter = 0;
					gettimeofday( &cAudioTimeoutStartTime, 0 );
					
					getExpectedPacketValues( aMessagePacket, &cNextAudioTrackID, &cNextAudioSampleID, 
											&cNextAudioBufferRangeStartID, &cNextAudioBufferRangeEndID );
					cAudioMessageList.push_back( aMessagePacket );
					
					}
				else if( aMessagePacket.messageHeader.AudioSample.SampleID == cNextAudioSampleID
						&& aMessagePacket.messageHeader.AudioSample.TrackID == cNextAudioTrackID )
					{
					Logger::logNotice( "Successfully added an audio packet to queue when queue wasn't empty" );
					Logger::logNotice( "SampleID: %d - cNextAudioSampleID: %d", aMessagePacket.messageHeader.AudioSample.SampleID, cNextAudioSampleID );
					Logger::logNotice( "TrackID: %d - cNextAudioTrackID: %d", aMessagePacket.messageHeader.AudioSample.TrackID, cNextAudioTrackID );
					
					cAudioPacketCounter = 0;
					gettimeofday( &cAudioTimeoutStartTime, 0 );
					
					getExpectedPacketValues( aMessagePacket, &cNextAudioTrackID, &cNextAudioSampleID, 
											&cNextAudioBufferRangeStartID, &cNextAudioBufferRangeEndID );
					cAudioMessageList.push_back( aMessagePacket );
					
					cAudioMessageListExtraPackets.sort();
					while( !cAudioMessageListExtraPackets.empty() )
						{
						if( cAudioMessageListExtraPackets.front().messageHeader.AudioSample.SampleID 
								< cNextAudioSampleID || cNextAudioTrackID > 
								cAudioMessageListExtraPackets.front().messageHeader.AudioSample.TrackID )
							{
							Logger::logWarning( "I Poppped the Front of my extra audio list" );
							cAudioMessageListExtraPackets.pop_front();
							}
						else if( cAudioMessageListExtraPackets.front().messageHeader.AudioSample.SampleID 
								==  cNextAudioSampleID && cNextAudioTrackID ==
								cAudioMessageListExtraPackets.front().messageHeader.AudioSample.TrackID )
							{
							Logger::logWarning( "I put the front of my extra list into the back of my regular list" );
							getExpectedPacketValues( cAudioMessageListExtraPackets.front(), &cNextAudioTrackID, &cNextAudioSampleID, 
													&cNextAudioBufferRangeStartID, &cNextAudioBufferRangeEndID );
							
							cAudioMessageList.push_back( cAudioMessageListExtraPackets.front() );
							
							cAudioMessageListExtraPackets.pop_front();
							}
						else
							{
							Logger::logWarning( "List from front to back" );
							while( !cAudioMessageListExtraPackets.empty() )
								{
								Logger::logWarning( "Audiotrack: %d - AudioSample: %d",
													cAudioMessageListExtraPackets.front().messageHeader.AudioSample.TrackID,
													cAudioMessageListExtraPackets.front().messageHeader.AudioSample.SampleID );
								cAudioMessageListExtraPackets.pop_front();
								}
							Logger::logWarning( "I BROKE UNDER THE PRESSURE" );
							break;
							}
						}
					}
				else
					{
					Logger::logNotice( "---Failed to add to queue---" );
					Logger::logNotice( "SampleID: %d - cNextAudioSampleID: %d", aMessagePacket.messageHeader.AudioSample.SampleID, cNextAudioSampleID );
					Logger::logNotice( "TrackID: %d - cNextAudioTrackID: %d", aMessagePacket.messageHeader.AudioSample.TrackID, cNextAudioTrackID );
					Logger::logNotice( "---Failed to add to queue---" );
					
					cAudioPacketCounter++;
					cAudioMessageListExtraPackets.push_back( aMessagePacket );
					}
			
				if( cNextAudioTrackID != -1 )
					{
					
					if( cAudioPacketCounter > NUMBER_OF_ACCEPTABLE_EXTRA_PACKETS ) 
						{
						sendMessageToServer( ControlBytes::RESEND_AUDIO, cNextAudioTrackID, cNextAudioSampleID,
											 cNextAudioBufferRangeStartID, cNextAudioBufferRangeEndID );
						
						Logger::logWarning( "Too many packets have arrived out of order, need to initiate AUDIORESEND" );
						Logger::logNotice( "AudioPacketCounter: %d", cAudioPacketCounter );
						Logger::logNotice( "Sent a RESEND_AUDIO packet to server." );					
						}
					}
				
				break;
			
			default:
				if( cNextVolumeTrackID == -1 )
					{
					Logger::logNotice( "Successfully added an audio packet to queue when queue WAS empty" );
					Logger::logNotice( "SampleID: %d - cNextVolumeSampleID: %d", aMessagePacket.messageHeader.VolumeSample.SampleID, cNextVolumeSampleID );
					Logger::logNotice( "TrackID: %d - cNextVolumeTrackID: %d", aMessagePacket.messageHeader.VolumeSample.TrackID, cNextVolumeTrackID );
					
					cVolumePacketCounter = 0;
					gettimeofday( &cVolumeTimeoutStartTime, 0 );
					
					getExpectedPacketValues( aMessagePacket, &cNextVolumeTrackID, &cNextVolumeSampleID, 
											&cNextVolumeBufferRangeStartID, &cNextVolumeBufferRangeEndID );
					cVolumeMessageList.push_back( aMessagePacket );
					
					}
				else if( aMessagePacket.messageHeader.VolumeSample.SampleID == cNextVolumeSampleID
						&& aMessagePacket.messageHeader.VolumeSample.TrackID == cNextVolumeTrackID )
					{
					Logger::logNotice( "Successfully added an Volume packet to queue when queue wasn't empty" );
					Logger::logNotice( "SampleID: %d - cNextVolumeSampleID: %d", aMessagePacket.messageHeader.VolumeSample.SampleID, cNextVolumeSampleID );
					Logger::logNotice( "TrackID: %d - cNextVolumeTrackID: %d", aMessagePacket.messageHeader.VolumeSample.TrackID, cNextVolumeTrackID );
					
					cVolumePacketCounter = 0;
					gettimeofday( &cVolumeTimeoutStartTime, 0 );
					
					getExpectedPacketValues( aMessagePacket, &cNextVolumeTrackID, &cNextVolumeSampleID, 
											&cNextVolumeBufferRangeStartID, &cNextVolumeBufferRangeEndID );
					cVolumeMessageList.push_back( aMessagePacket );
					
					cVolumeMessageListExtraPackets.sort();
					while( !cVolumeMessageListExtraPackets.empty() )
						{
						if( cVolumeMessageListExtraPackets.front().messageHeader.VolumeSample.SampleID 
								< cNextVolumeSampleID || cNextVolumeTrackID > 
								cVolumeMessageListExtraPackets.front().messageHeader.VolumeSample.TrackID )
							{
							Logger::logWarning( "I Poppped the Front of my extra Volume list" );
							cVolumeMessageListExtraPackets.pop_front();
							}
						else if( cVolumeMessageListExtraPackets.front().messageHeader.VolumeSample.SampleID 
								==  cNextVolumeSampleID && cNextVolumeTrackID ==
								cVolumeMessageListExtraPackets.front().messageHeader.VolumeSample.TrackID )
							{
							Logger::logWarning( "I put the front of my extra list into the back of my regular list" );
							getExpectedPacketValues( cVolumeMessageListExtraPackets.front(), &cNextVolumeTrackID, &cNextVolumeSampleID, 
													&cNextVolumeBufferRangeStartID, &cNextVolumeBufferRangeEndID );
							
							cVolumeMessageList.push_back( cVolumeMessageListExtraPackets.front() );
							
							cVolumeMessageListExtraPackets.pop_front();
							}
						else
							{
							Logger::logWarning( "List from front to back" );
							while( !cVolumeMessageListExtraPackets.empty() )
								{
								Logger::logWarning( "Volumetrack: %d - VolumeSample: %d",
													cVolumeMessageListExtraPackets.front().messageHeader.VolumeSample.TrackID,
													cVolumeMessageListExtraPackets.front().messageHeader.VolumeSample.SampleID );
								cVolumeMessageListExtraPackets.pop_front();
								}
							Logger::logWarning( "I BROKE UNDER THE PRESSURE" );
							break;
							}
						}
					}
				else
					{
					Logger::logNotice( "---Failed to add to queue---" );
					Logger::logNotice( "SampleID: %d - cNextVolumeSampleID: %d", aMessagePacket.messageHeader.VolumeSample.SampleID, cNextVolumeSampleID );
					Logger::logNotice( "TrackID: %d - cNextVolumeTrackID: %d", aMessagePacket.messageHeader.VolumeSample.TrackID, cNextVolumeTrackID );
					Logger::logNotice( "---Failed to add to queue---" );
					
					cVolumePacketCounter++;
					cVolumeMessageListExtraPackets.push_back( aMessagePacket );
					}
			
				if( cNextVolumeTrackID != -1 )
					{
					
					if( cVolumePacketCounter > NUMBER_OF_ACCEPTABLE_EXTRA_PACKETS ) 
						{
						sendMessageToServer( ControlBytes::RESEND_VOLUME, cNextVolumeTrackID, cNextVolumeSampleID,
											 cNextVolumeBufferRangeStartID, cNextVolumeBufferRangeEndID );
						
						Logger::logWarning( "Too many packets have arrived out of order, need to initiate VolumeRESEND" );
						Logger::logNotice( "VolumePacketCounter: %d", cVolumePacketCounter );
						Logger::logNotice( "Sent a RESEND_VOLUME packet to server." );					
						}
					}
				
				break;
			}
		
		return 0;
		}
		
	void PlaybackClient::getExpectedPacketValues(	MESSAGEPACKET aMessagePacket,
													Networking::trackid_t *aTrackID,
													Networking::sampleid_t *aSampleID,
													Networking::sampleid_t *aBufferRangeStartID,
													Networking::sampleid_t *aBufferRangeEndID,
													timeval *aTimeoutStartTime )
		{
			
		if( aMessagePacket.messageHeader.AudioSample.TrackID + 1 <= aMessagePacket.messageHeader.AudioSample.BufferRangeEndID )
			{
			*aTrackID = aMessagePacket.messageHeader.AudioSample.TrackID + 1;
			*aSampleID = aMessagePacket.messageHeader.AudioSample.SampleID;
			*aBufferRangeStartID = aMessagePacket.messageHeader.AudioSample.BufferRangeStartID;
			*aBufferRangeEndID = aMessagePacket.messageHeader.AudioSample.BufferRangeEndID;
			gettimeofday( aTimeoutStartTime, 0 );
			}
		else
			{
			*aTrackID = -1;
			*aSampleID = -1;
			*aBufferRangeStartID = -1;
			*aBufferRangeEndID = -1;
			}
		}
	}
	
void handleCtrlCSignal( int aPlaceholder )
	{
	Logger::logWarning( "Closed by a Ctrl-C signal" );
	Logger::closeLogFile();
	exit( SIGINT );
	}
	
int main( int argc, char **argv )
	{
	if( argc != 6 )
		{
		std::cout << "\nError:Incorrect usage\n" << std::endl;
		std::cout << argv[0] << " <server IP address> <server port #> <client x position> <client y position> <client IP Address>" << std::endl;
		std::cout << "\ni.e.\n\n" << argv[0] << " 127.0.1.1 2222 5.2 8.3 72.161.218.139\n" << std::endl;
		std::cout << "\ni.e.\n\n" << argv[0] << " 127.0.0.1 4688 5.2 8.3 127.0.0.1\n" << std::endl;

		return -1;
		}
	
	signal( SIGINT, handleCtrlCSignal );
	Networking::PlaybackClient mPlaybackClient( argv[1], argv[2], atof( argv[3] ), atof( argv[4] ), argv[5] );
	
	return 0;
	}

#endif