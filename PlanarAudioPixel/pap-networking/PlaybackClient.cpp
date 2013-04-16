#ifdef PLAYBACKCLIENT
#include "PlaybackClient.h"
#include "Socket.h"
#include "NetworkGlobals.h"
#include "ControlByteConstants.h"
#include "NetworkConstants.h"

/******************************Needs Work******************************/

namespace Networking 
	{
	
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
	
	bool operator > ( MESSAGEPACKET messagePacket1, MESSAGEPACKET messagePacket2 )
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
			cNextVolumeBufferRangeEndID( -1 ),
			cFirstAudioPacket( true ),
			cIsAudioCaughtUp( false )
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
		
		while( cSocketData = socket( AF_INET, SOCK_STREAM, 0 ) < 0 )
			{
			Logger::logWarning( "ERROR opening socket" );
			}
		
		memset( &cServer, 0, sizeof( cServer ) );
		cServer.sin_family = AF_INET;
		
		cHostEntry = gethostbyname( aHostName );
		if( cHostEntry == NULL )
			{
			Logger::logWarning( "ERROR: No such host" );
			Logger::closeLogFile();
			exit( 0 );
			}
		
		memcpy( &(cServer.sin_addr.s_addr), cHostEntry->h_addr, cHostEntry->h_length);
		cServer.sin_port = htons( atoi( aPortNumber ) );
		
		while( connect( cSocketData, (struct sockaddr *)&cServer , sizeof( cServer ) ) < 0 )
			{
			Logger::logWarning( "ERROR connecting" );
			}
			
		Logger::logNotice( "Successfully connected to server" );
		
		cListenerThread = new boost::thread( boost::bind ( &Networking::PlaybackClient::listenerFunction, this ) );
		//cPlaybackThread = new boost::thread( boost::bind ( &Networking::PlaybackClient::playbackFunction, this ) );
		playbackFunction();
		}
		
	void PlaybackClient::listenerFunction()
		{
		int mNewMessages;
						
		do
			{
				
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
				//mMessage.TransportControl.clientID = stringToGuid( cBroadcastIP, cLocalIP );
				mMessage.TransportControl.requestID = aRequestID;
				
				Logger::logNotice( "Created a TransportControl packet." );
				break;
				
			default:
				/*
				All other client to server messages have the following data types
				*/
				//mMessage.ClientCheckIn.clientID = stringToGuid( cBroadcastIP, cLocalIP );
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
		
		recieveMessageFromServer(); 
		
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
				AddAudioOrVolumePacketToList( mTempNetworkPacket );
				break;
			
			case ControlBytes::BEGIN_PLAYBACK:
			case ControlBytes::STOP_PLAYBACK:
			case ControlBytes::PAUSE_PLAYBACK:
				Logger::logNotice( "Received a TransportControl packet." );
				
				changePlaybackStatus( mPlaceHolder->messageHeader.ControlByte );
				break;
				
			default:
				Logger::logNotice( "Received a network message packet." );
				
				memcpy( &mTempNetworkMessage, mPlaceHolder, sizeof( PacketStructures::NetworkMessage ) );
				cNetworkMessageQueue.push( mTempNetworkMessage );
			}
		
		return 1;
		}
		
	int PlaybackClient::AddAudioOrVolumePacketToList( MESSAGEPACKET aMessagePacket )
		{
		/*
		Make all method variables class/state variables to improve packet dropped checks.
		*/
			
		switch( aMessagePacket.messageHeader.ControlByte )
			{
			case ControlBytes::SENDING_AUDIO:
				if( cFirstAudioPacket )
					{
					cFirstAudioPacket = false;
					checkForMissingAudioHeader( aMessagePacket );
					}
								
				cAudioMessageList.push_back( aMessagePacket );
				
				if( !cIsAudioCaughtUp )
					{
					cAudioMessageList.sort();
					if( aMessagePacket.messageHeader.AudioSample.TrackID == cNextAudioTrackID
						&& aMessagePacket.messageHeader.AudioSample.SampleID == cNextAudioSampleID )
						{
						cIsAudioCaughtUp = true;
						}
					}
				
				Logger::logNotice( "Added audio packet to queue" );
				break;
			
			default:
				cVolumeMessageList.push_back( aMessagePacket );
				Logger::logNotice( "Added volume packet to queue" );
				break;
			}
		
		return 0;
		}
		
	void PlaybackClient::checkForMissingAudioHeader( MESSAGEPACKET aMessagePacket )
		{
		if( aMessagePacket.messageHeader.AudioSample.SampleID != 0 )
			{
			for( int i = 0; i < AUDIOHEADERRANGE; ++i )
				{
				sendMessageToServer( ControlBytes::RESEND_AUDIO, 
									aMessagePacket.messageHeader.AudioSample.TrackID,
									i,
									aMessagePacket.messageHeader.AudioSample.BufferRangeStartID, 
									aMessagePacket.messageHeader.AudioSample.BufferRangeEndID );
				cNextAudioSampleID = i;
				}
			
			cNextAudioTrackID = aMessagePacket.messageHeader.AudioSample.TrackID;
			}
		else
			{
			cIsAudioCaughtUp = true;
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