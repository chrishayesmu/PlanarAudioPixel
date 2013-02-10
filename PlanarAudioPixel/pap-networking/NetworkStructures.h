// -----------------------------------------------------------------
// This file contains a number of data structures that will be in use
// for most or all of the networking code. Most of these data structures
// are outlined in section 3.1 of the PAP document.
//
// Author: Chris Hayes - left out pragma once. gg
// -----------------------------------------------------------------
#pragma once

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

	// A struct for representing positions, for compactness and in case we need
	// to easily increase the precision of the position.
	struct PositionInfo
	{
		float x;
		float y;
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

		// The client's position in the grid, stored as an offset from the top-left corner of the grid,
		// which has position (0, 0).
		PositionInfo Offset;

		// The 8-byte Unix timestamp of when the client last checked in with the server.
		unsigned long LastCheckInTime;
	};

	typedef unsigned char trackid_t;
	typedef unsigned int sampleid_t;

	// A structure containing the information regarding an audio sample that is sent to
	// clients to be played.
	struct AudioSample 
	{
		// The ID of the sample.
		sampleid_t SampleID;

		// The offset, in microseconds, that this sample should be played.
		time_t TimeOffset;

		// The information regarding the raw data of this audio sample.
		IO::AudioData Data;
	};

	// A structure containing volume information regarding an audio sample for a particular
	// client.
	struct VolumeInfo 
	{
		// The ID of the track to which this information applies.
		trackid_t TrackID;

		// The ID of the sample to which this information applies.
		sampleid_t SampleID;

		// The GUID of the client to which this information applies.
		ClientGUID ClientID;

		// The volume for this sample to be played at, between 0 and 1.
		float Volume;
	};

	// A typedef for an AudioBuffer object, which is a map between sample IDs and
	// audio objects.
	typedef std::map<sampleid_t, AudioSample> AudioBuffer;

	// A typedef for an VolumeBuffer object, which is a map between sample IDs and
	// maps between ClientGUIDs and VolumeInfo objects.
	typedef std::map<sampleid_t, std::map<ClientGUID, VolumeInfo>> VolumeBuffer;

	// A typedef for a PositionBuffer object, which is a map from sample IDs to
	// PositionInfo objects
	typedef std::map<sampleid_t, PositionInfo> PositionBuffer;

	// A structure containing information regarding a particular track.
	struct TrackInfo {

		// The ID of the track.
		trackid_t TrackID;

		// Where this track is in its playback, in microseconds.
		time_t currentPlaybackOffset;

		// The length, in microseconds, of this track.
		time_t trackLength;
		
		// A buffer for the raw audio data for this track.
		AudioBuffer audioData;

		// A buffer for the position data for each sample.
		PositionBuffer positionData;

		// A buffer for the volume data for this track.
		VolumeBuffer volumeData;

	};

	// A typedef for a TrackBuffer object, which is a map between track IDs and
	// TrackInfo objects
	typedef std::map<trackid_t, TrackInfo> TrackBuffer;

}