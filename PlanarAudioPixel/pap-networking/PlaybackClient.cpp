#include "PlaybackClient.h"
#include "Socket.h"
#include "NetworkGlobals.h"
#include "ControlByteConstants.h"
#include "NetworkConstants.h"

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
			cYPosition( aYPosition )
		{
		cSocketData = socket( AF_INET, SOCK_DGRAM, 0 );
		cServer.sin_family = AF_INET;
		
		/*
		Get local machine info
		*/
		gethostname( cBroadcastIP, SIZE_OF_IP_INET_ADDRESSES );
		printf( "----UDPClient1 running at host NAME: %s\n", cBroadcastIP );
		cHp = gethostbyname( cBroadcastIP );
		bcopy( cHp->h_addr, &( cServer.sin_addr ), cHp->h_length );
		printf( "(UDPClient1 INET ADDRESS is: %s )\n", inet_ntoa( cServer.sin_addr ) );
		strncpy( cLocalIP, inet_ntoa( cServer.sin_addr ), SIZE_OF_IP_INET_ADDRESSES );

		cHp = gethostbyname( aHostName );
		bcopy( cHp->h_addr, &(cServer.sin_addr.s_addr), cHp->h_length);
		cServer.sin_port = htons( atoi( aPortNumber ) );
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
		time_t mStart;
		PacketStructures::NetworkMessage mConnectionMessage, mServerResponseMessage;
		mConnectionMessage.ControlByte = ControlBytes::NEW_CONNECTION;
		mConnectionMessage.ClientConnection.clientID = stringToGuid( cBroadcastIP, cLocalIP );
		mConnectionMessage.ClientConnection.position.x = cXPosition;
		mConnectionMessage.ClientConnection.position.y = cYPosition;
		do
			{
			sendto( cSocketData, &mConnectionMessage, sizeof( PacketStructures::NetworkMessage ), 0, (const sockaddr*)&cServer, sizeof( cServer ) );
			
			mStart = time( NULL );
			while( time( NULL ) - mStart > cClientReceivedPacketTimeout )
				{
				if( recieveMessageFromServer() != -1 )
					{
					break;
					}
				}
				
			if( mRecieve != -1 )
				{
				if( cIncomingMessage.messageHeader.ControlByte == ControlBytes::NEW_CONNECTION )
					{
					return;
					}
				}
			}while( 1 );		
		}
	
	/*
	This function sends a check in message to the server. Nothing else.
	*/
	void PlaybackClient::checkInWithServer()
		{
		PacketStructures::NetworkMessage mCheckInMessage;
		mCheckInMessage.ControlByte = ControlBytes::PERIODIC_CHECK_IN;
		mCheckInMessage.ClientCheckIn.clientID = stringToGuid( cBroadcastIP, cLocalIP );
		mCheckInMessage.ClientCheckIn.position.x = cXPosition;
		mCheckInMessage.ClientCheckIn.position.y = cYPosition;
		sendto( cSocketData, &mCheckInMessage, sizeof( PacketStructures::NetworkMessage ), 0, (const sockaddr*)&cServer, sizeof( cServer ) );
		}
	
	}

int main( int argc, char **argv )
		{
		if( argc != 5 )
			{
			std::cout << "\nError:Incorrect usage\n" << std::endl;
			std::cout << argv[0] << " <server IP address> <server port #> <client x position> <client y position>" << std::endl;
			std::cout << "\ni.e.\n" << argv[0] << " 123.456.789.1011 2222 5.2 8.3\n" << std::endl;
			return -1;
			}
		Networking::PlaybackClient mPlaybackClient( argv[1], argv[2], atof( argv[3] ), atof( argv[4] ) );
		return 0;
		}