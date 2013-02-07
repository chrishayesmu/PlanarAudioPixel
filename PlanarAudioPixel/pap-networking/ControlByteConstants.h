// -----------------------------------------------------------------
// This file contains the control bytes which are sent along with
// every UDP packet to identify what the packet contains. These constants
// are established in the PAP document in section 3.1.
//
// Author: Chris Hayes
// -----------------------------------------------------------------

namespace Networking
{
	// Packet identifier that is sent when a client requests to establish a connection, 
	// or when the server accepts the connection.
	const unsigned char NEW_CONNECTION = 0x01;

	// Packet identifier that is sent when the server informs a client that it is being disconnected.
	const unsigned char DISCONNECT = 0x02;

	// Packet identifier that is sent when the client makes a periodic check-in with the server.
	const unsigned char PERIODIC_CHECK_IN = 0x03;

	// Packet identifier that is sent when the server sends audio data to a client.
	const unsigned char SENDING_AUDIO = 0x04;

	// Packet identifier that is sent when the server sends volume data to a client.
	const unsigned char SENDING_VOLUME = 0x05;

	// Packet identifier that is sent when a client needs to request a resending of audio data.
	const unsigned char RESEND_AUDIO = 0x06;

	// Packet identifier that is sent when a client needs to request a resending of volume data.
	const unsigned char RESEND_VOLUME = 0x07;

	// Packet identifier that is sent when the server removes a track from playback
	// and informs a client to dump the associated audio and volume data.
	const unsigned char REMOVE_TRACK = 0x08;

	// Packet identifier that is sent when the server tells clients to pause playback, 
	// and when a client sends an acknowledgement to the server that playback has paused.
	const unsigned char PAUSE_PLAYBACK = 0x09;

	// Packet identifier that is sent when the server tells clients to resume playback, 
	// and when a client sends an acknowledgement to the server that playback has resumed.
	const unsigned char RESUME_PLAYBACK = 0x0a;

	// Packet identifier that is sent when the server tells clients to stop playback, 
	// and when a client sends an acknowledgement to the server that playback has stopped.
	const unsigned char STOP_PLAYBACK = 0x0b;
	
	// Packet identifier that is sent when a client asks the server to help with synchronizing
	// latency, and when the server acknowledges a latency request.
	const unsigned char SYNCHRONIZATION_REQUEST = 0x0d;
}