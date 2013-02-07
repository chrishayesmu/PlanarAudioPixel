// -----------------------------------------------------------------
// This file contains a number of data structures that will be in use
// for most or all of the networking code. Most of these data structures
// are outlined in section 3.1 of the PAP document.
//
// Author: Chris Hayes
// -----------------------------------------------------------------

#include <map>
#include "../pap-file-io/IOStructures.h"

namespace Networking
{
	// A structure for representing IP addresses, as either a 4-byte integer
	// or as 4 separate bytes, for convenience.
	union IP_Address
	{
		// The 4-byte IP address, stored as an integer.
		unsigned int RawIP;

		struct
		{
			// The first byte of the IP address, as read from left-to-right.
			unsigned char Byte1;

			// The second byte of the IP address, as read from left-to-right.
			unsigned char Byte2;

			// The third byte of the IP address, as read from left-to-right.
			unsigned char Byte3;

			// The fourth byte of the IP address, as read from left-to-right.
			unsigned char Byte4;
		};
	};

	// A structure for representing a client's globally unique ID (GUID). This 
	// structure is mostly used for creating a mapping to clients for the
	// Client Information Table.
	struct ClientGUID
	{
		IP_Address BroadcastIP;
		IP_Address LocalIP;
	};

	// A structure for representing a client's position and networking information.
	// Defined in section 3.1 of the PAP document.
	struct Client
	{
		// The broadcast IP of the client, generally the IP of the client's router or network switch.
		IP_Address BroadcastIP;

		// The local IP of the client, generally the internal IP of the client on its network.
		IP_Address LocalIP;

		// The speaker's X offset from the top-left corner of the speaker grid.
		float OffsetX;

		// The speaker's Y offset from the top-left corner of the speaker grid.
		float OffsetY;

		// The 8-byte Unix timestamp of when the client last checked in with the server.
		unsigned long LastCheckInTime;
	};

	// A structure containing the information regarding an audio sample that is sent to
	// clients to be played.
	struct AudioSample 
	{
		// The ID of the track to which this sample belongs.
		unsigned char TrackID;

		// The ID of the sample.
		unsigned int SampleID;

		// The offset, in microseconds, that this sample should be played.
		unsigned int TimeOffset;

		// The information regarding the raw data of this audio sample.
		IO::AudioData Data;

	};

	// A structure containing volume information regarding an audio sample for a particular
	// client.
	struct VolumeInfo 
	{
		// The ID of the track to which this information applies.
		unsigned char TrackID;

		// The ID of the sample to which this information applies.
		unsigned int SampleID;

		// The ID of the client to which this information applies.
		ClientGUID ClientID;

		// The volume for this sample to be played at, between 0 and 1.
		float Volume;

	};

	// A typedef for an AudioBuffer object, which is a map between sample IDs and
	// audio objects.
	typedef std::map<int, AudioSample> AudioBuffer;

	// A typedef for an VolumeBuffer object, which is a map between sample IDs and
	// VolumeInfo objects.
	typedef std::map<int, VolumeInfo> VolumeBuffer;
}