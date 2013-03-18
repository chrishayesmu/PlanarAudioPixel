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
	PlaybackClient::PlaybackClient( char *aHostName, char *aPortNumber, float aXPosition, float aYPosition )
		: 	cXPosition( aXPosition ),
			cYPosition( aYPosition ),
			cPlay( false ),
			cPause( false )
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
		strncpy( cBroadcastIP, inet_ntoa( cServer.sin_addr ), SIZE_OF_IP_INET_ADDRESSES );

		/*
		Segmentation fault will occur with bad aHostName
		*/
		cHostEntry = gethostbyname( aHostName );
		memcpy( &(cServer.sin_addr.s_addr), cHostEntry->h_addr, cHostEntry->h_length);
		cServer.sin_port = htons( atoi( aPortNumber ) );
		
		/*
		connectToServer();
		cListenerThread = new boost::thread( boost::bind ( &Networking::PlaybackClient::listenerFunction, this ) );
		playbackFunction();
		g++ PlaybackClient.cpp -lboost_thread-mt
		*/
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
		int mRecieve;
		timeval mTimeoutTime, mCurrentTime;
		Logger::logNotice( "Connecting to server" );
		
		do
			{
			Logger::logNotice( "Sending NEW_CONNECTION packet" );
			sendMessageToServer( ControlBytes::NEW_CONNECTION );
			
			gettimeofday( &mTimeoutTime, 0 );
			gettimeofday( &mCurrentTime, 0 );
			
			while( ( mCurrentTime.tv_sec - mTimeoutTime.tv_sec ) * 1000 + ( mCurrentTime.tv_usec - mTimeoutTime.tv_usec ) / 1000 
					< PACKETRECEIPTTIMEOUT )
				{
				if( ( mRecieve = recieveMessageFromServer() ) != -1 )
					{
					break;
					}
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
		timeval mCheckInTime, mCurrentTime;
		gettimeofday( &mCheckInTime, 0 );
		
		do
			{
			gettimeofday( &mCurrentTime, 0 );
			if( ( mCurrentTime.tv_sec - mCheckInTime.tv_sec ) * 1000 + ( mCurrentTime.tv_usec - mCheckInTime.tv_usec ) / 1000 
					> CLIENT_CHECKIN_DELAY )
				{
				Logger::logNotice( "Initiating periodic check in with server" );
				checkInWithServer();
				Logger::logNotice( "PERIODIC_CHECK_IN packet sent" );
				gettimeofday( &mCheckInTime, 0 );
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
					//boost::mutex::scoped_lock lock( cPlaybackLock );
					cPlay = true;
					Logger::logNotice( "Started audio playback."  );
					}
				break;
			case ControlBytes::STOP_PLAYBACK:
					{
					//boost::mutex::scoped_lock lock( cPlaybackLock );
					cPlay = false;
					Logger::logNotice( "Stopped audio playback."  );
					}
				break;
			case ControlBytes::PAUSE_PLAYBACK:
					{
					//boost::mutex::scoped_lock lock( cPlaybackLock );
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
			if( !cAudioMessageList.empty() )
				{
				Logger::logNotice( "Handled an Audio message."  );
				cAudioMessageList.pop_front();
				}
			
			if( !cVolumeMessageList.empty() )
				{
				Logger::logNotice( "Handled a Volume message."  );
				cVolumeMessageList.pop_front();
				}
			
			if( !cNetworkMessageQueue.empty() )
				{
				Logger::logNotice( "Handled a Network message."  );
				
				if( cNetworkMessageQueue.front().ControlByte == ControlBytes::DISCONNECT )
					{
					Logger::logNotice( "Recieved exit message from server. Exiting playback client." );
					Logger::closeLogFile();
					exit( ControlBytes::DISCONNECT );
					}

				cNetworkMessageQueue.pop();
				}
			
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
		static int mAudioPacketCounter = 0, mVolumePacketCounter = 0;
		static timeval mAudioTimeoutStartTime, mVolumeTimeoutStartTime, mCurrentTime;
		static Networking::trackid_t mNextAudioTrackID = -1, mNextVolumeTrackID = -1;
		static Networking::sampleid_t mNextAudioSampleID = -1, mNextAudioBufferRangeStartID = -1, mNextAudioBufferRangeEndID = -1;
		static Networking::sampleid_t mNextVolumeSampleID = -1, mNextVolumeBufferRangeStartID = -1, mNextVolumeBufferRangeEndID = -1;
			
		switch( aMessagePacket.messageHeader.ControlByte )
			{
			case ControlBytes::SENDING_AUDIO:
				if( mNextAudioTrackID == -1 )
					{
					mAudioPacketCounter = 0;
					getExpectedPacketValues( aMessagePacket, &mNextAudioTrackID, &mNextAudioSampleID, 
											&mNextAudioBufferRangeStartID, &mNextAudioBufferRangeEndID );
					cAudioMessageList.push_back( aMessagePacket );
					
					}
				else if( aMessagePacket.messageHeader.AudioSample.SampleID == mNextAudioSampleID
						&& aMessagePacket.messageHeader.AudioSample.TrackID == mNextAudioTrackID )
					{
					mAudioPacketCounter = 0;
					getExpectedPacketValues( aMessagePacket, &mNextAudioTrackID, &mNextAudioSampleID, 
											&mNextAudioBufferRangeStartID, &mNextAudioBufferRangeEndID );
					cAudioMessageList.push_back( aMessagePacket );
					
					cAudioMessageListExtraPackets.sort();
					while( !cAudioMessageListExtraPackets.empty() )
						{
						if( cAudioMessageListExtraPackets.front().messageHeader.AudioSample.SampleID 
								< mNextAudioSampleID || mNextAudioTrackID > 
								cAudioMessageListExtraPackets.front().messageHeader.AudioSample.TrackID )
							{
							cAudioMessageListExtraPackets.pop_front();
							}
						else if( cAudioMessageListExtraPackets.front().messageHeader.AudioSample.SampleID 
								==  mNextAudioSampleID && mNextAudioTrackID ==
								cAudioMessageListExtraPackets.front().messageHeader.AudioSample.TrackID )
							{
							getExpectedPacketValues( cAudioMessageListExtraPackets.front(), &mNextAudioTrackID, &mNextAudioSampleID, 
													&mNextAudioBufferRangeStartID, &mNextAudioBufferRangeEndID );
							
							cAudioMessageList.push_back( cAudioMessageListExtraPackets.front() );
							
							cAudioMessageListExtraPackets.pop_front();
							}
						else
							{
							break;
							}
						}
					}
				else
					{
					mAudioPacketCounter++;
					cAudioMessageListExtraPackets.push_back( aMessagePacket );
					}
			
				if( mNextAudioTrackID != -1 )
					{
					gettimeofday( &mCurrentTime, 0 );
					
					if( mAudioPacketCounter > NUMBER_OF_ACCEPTABLE_EXTRA_PACKETS
						|| ( mCurrentTime.tv_sec - mAudioTimeoutStartTime.tv_sec ) * 1000 
						+ ( mCurrentTime.tv_usec - mAudioTimeoutStartTime.tv_usec ) / 1000 
						> PACKETRECEIPTTIMEOUT ) 
						{
						sendMessageToServer( ControlBytes::RESEND_AUDIO,
											cAudioMessageList.back().messageHeader.AudioSample.TrackID,
											cAudioMessageList.back().messageHeader.AudioSample.SampleID + 1,
											cAudioMessageList.back().messageHeader.AudioSample.BufferRangeStartID,
											cAudioMessageList.back().messageHeader.AudioSample.BufferRangeEndID );	
						Logger::logNotice( "Sent a RESEND_AUDIO packet to server." );					
						}
					}
				
				break;
			
			default:
				if( mNextVolumeTrackID == -1 )
					{
					mVolumePacketCounter = 0;
					getExpectedPacketValues( aMessagePacket, &mNextVolumeTrackID, &mNextVolumeSampleID, 
											&mNextVolumeBufferRangeStartID, &mNextVolumeBufferRangeEndID );
					cVolumeMessageList.push_back( aMessagePacket );
					
					}
				else if( aMessagePacket.messageHeader.VolumeSample.SampleID == mNextVolumeSampleID
						&& aMessagePacket.messageHeader.VolumeSample.TrackID == mNextVolumeTrackID )
					{
					mVolumePacketCounter = 0;
					getExpectedPacketValues( aMessagePacket, &mNextVolumeTrackID, &mNextVolumeSampleID, 
											&mNextVolumeBufferRangeStartID, &mNextVolumeBufferRangeEndID );
					cVolumeMessageList.push_back( aMessagePacket );
					
					cVolumeMessageListExtraPackets.sort();
					while( !cVolumeMessageListExtraPackets.empty() )
						{
						if( cVolumeMessageListExtraPackets.front().messageHeader.VolumeSample.SampleID 
								< mNextVolumeSampleID || mNextVolumeTrackID > 
								cVolumeMessageListExtraPackets.front().messageHeader.VolumeSample.TrackID )
							{
							cVolumeMessageListExtraPackets.pop_front();
							}
						else if( cVolumeMessageListExtraPackets.front().messageHeader.VolumeSample.SampleID 
								==  mNextVolumeSampleID && mNextVolumeTrackID ==
								cVolumeMessageListExtraPackets.front().messageHeader.VolumeSample.TrackID )
							{
							getExpectedPacketValues( cVolumeMessageListExtraPackets.front(), &mNextVolumeTrackID, &mNextVolumeSampleID, 
													&mNextVolumeBufferRangeStartID, &mNextVolumeBufferRangeEndID );
							
							cVolumeMessageList.push_back( cVolumeMessageListExtraPackets.front() );
							
							cVolumeMessageListExtraPackets.pop_front();
							}
						else
							{
							break;
							}
						}
					}
				else
					{
					mVolumePacketCounter++;
					cVolumeMessageListExtraPackets.push_back( aMessagePacket );
					}
			
				if( mNextVolumeTrackID != -1 )
					{
					gettimeofday( &mCurrentTime, 0 );
					
					if( mVolumePacketCounter > NUMBER_OF_ACCEPTABLE_EXTRA_PACKETS
						|| ( mCurrentTime.tv_sec - mVolumeTimeoutStartTime.tv_sec ) * 1000 
						+ ( mCurrentTime.tv_usec - mVolumeTimeoutStartTime.tv_usec ) / 1000 
						> PACKETRECEIPTTIMEOUT ) 
						{
						sendMessageToServer( ControlBytes::RESEND_AUDIO,
											cVolumeMessageList.back().messageHeader.VolumeSample.TrackID,
											cVolumeMessageList.back().messageHeader.VolumeSample.SampleID + 1,
											cVolumeMessageList.back().messageHeader.VolumeSample.BufferRangeStartID,
											cVolumeMessageList.back().messageHeader.VolumeSample.BufferRangeEndID );	
						Logger::logNotice( "Sent a RESEND_Volume packet to server." );					
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
			
		if( aMessagePacket.messageHeader.AudioSample.SampleID + 1 <= aMessagePacket.messageHeader.AudioSample.BufferRangeEndID )
			{
			*aTrackID = aMessagePacket.messageHeader.AudioSample.SampleID;
			*aSampleID = aMessagePacket.messageHeader.AudioSample.SampleID + 1;
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
	
int main( int argc, char **argv )
		{
		if( argc != 5 )
			{
			std::cout << "\nError:Incorrect usage\n" << std::endl;
			std::cout << argv[0] << " <server IP address> <server port #> <client x position> <client y position>" << std::endl;
			std::cout << "\ni.e.\n" << argv[0] << " 127.0.1.1 2222 5.2 8.3\n" << std::endl;
			return -1;
			}
		
		
		Networking::PlaybackClient mPlaybackClient( argv[1], argv[2], atof( argv[3] ), atof( argv[4] ) );
		
		return 0;
		}