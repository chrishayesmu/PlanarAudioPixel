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
#include "ControlByteConstants.h"

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
		union {
			struct {
				IP_Address BroadcastIP;
				IP_Address LocalIP;
			};
			unsigned __int64 ID;
		};

		bool operator < (const ClientGUID& ID) const;
	};

	// A structure for representing a client's position and networking information.
	// Defined in section 3.1 of the PAP document.
	struct Client
	{

		// A union combining the BroadcastIP and LocalIP as a ClientGUID.
		union {
			struct {
				// The broadcast IP of the client, generally the IP of the client's router or network switch.
				IP_Address BroadcastIP;

				// The local IP of the client, generally the internal IP of the client on its network.
				IP_Address LocalIP;
			};
			ClientGUID ClientID;
		};

		// The client's position in the grid, stored as an offset from the top-left corner of the grid,
		// which has position (0, 0).
		PositionInfo Offset;

		// The 8-byte Unix timestamp of when the client last checked in with the server.
		time_t LastCheckInTime;
	};

	typedef unsigned int trackid_t;
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

	// A typedef for VolumeInfo, which is a map tying a client to a volume level.
	typedef std::map<ClientGUID, float> VolumeInfo;

	// A typedef for an AudioBuffer object, which is a map from sample IDs and
	// audio objects.
	typedef std::map<sampleid_t, AudioSample> AudioBuffer;

	// A typedef for an VolumeBuffer object, which is a map from sample IDs to
	// VolumeInfo objects, which themselves map ClientGUIDs to actual volume levels.
	typedef std::map<sampleid_t, VolumeInfo> VolumeBuffer;

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

		// The index of the last sample that was buffered
		unsigned int samplesBuffered;

		// A buffer for the position data for each sample.
		PositionBuffer positionData;
		
		// A buffer for the raw audio data for this track.
		AudioBuffer audioSamples;

		// A buffer for the volume data for this track.
		VolumeBuffer volumeData;

	};

	// A typedef for a TrackBuffer object, which is a map between track IDs and
	// TrackInfo objects
	typedef std::map<trackid_t, TrackInfo> TrackBuffer;

	
	namespace PacketStructures {

		// [24] A struct for specifying the volume of two clients for a volume data message.
		//		Two clients are packed into this struct to align on an 8-byte boundary and
		//		save 4 bytes per client while still maintaining a simple programming paradigm.
	/*
	2>  class ClientVolume	size(24):
	2>  	+---
	2>   0	| ClientGUID clientID_1
	2>   8	| float		 clientVolume_1
	2>  12	| float		 clientVolume_2
	2>  16	| ClientGUID clientID_2
	2>  	+---
	*/
		struct ClientVolume 
		{
			/* [8] */ ClientGUID clientID_1;
			/* [4] */ float  clientVolume_1;
			/* [4] */ float  clientVolume_2;
			/* [8] */ ClientGUID clientID_2;
		};

		// [32] Represents a network packet.
		struct NetworkMessage {
			
			// Every message on our network contains a control control byte
			/* [1] */ controlcode_t ControlByte;
			/* [7] explicit padding */ unsigned char _pad[7];

			// Message headers
			union {

				// [16] Client to Server - Connection notification
				struct {
					// The ID and position of the client
					/* [8] */ ClientGUID clientID;
					/* [8] */ PositionInfo position;
				} ClientConnection;

				// [16] Client to Server - Check-in
				struct {
					// The ID and position of the client
					/* [8] */ ClientGUID clientID;
					/* [8] */ PositionInfo position;
				} ClientCheckIn;
				
				// [16] Server to Clients - Audio sample header
				struct {
					/* [4] */ trackid_t TrackID;
					/* [4] */ sampleid_t SampleID;
					/* [4] */ sampleid_t BufferRangeStartID;
					/* [4] */ sampleid_t BufferRangeEndID;
				} AudioSample;
				
				// [16] Client to Server - Audio sample resend request
				struct {
					// The ID of the track and the ID of the sample that is to be resent
					/* [4] */ trackid_t TrackID;
					/* [4] */ sampleid_t SampleID;
					/* [8] explicit padding */ unsigned char _pad[8];
				} AudioResendRequest;
				
				// [16] Server to Clients - Volume sample header
				struct {
					/* [4] */ trackid_t TrackID;
					/* [4] */ sampleid_t SampleID;
					/* [4] */ sampleid_t BufferRangeStartID;
					/* [4] */ sampleid_t BufferRangeEndID;
				} VolumeSample;
				
				// [16] Client to Server - Audio sample resend request
				struct {
					// The ID of the track and the ID of the sample that is to be resent
					/* [4] */ trackid_t TrackID;
					/* [4] */ sampleid_t SampleID;
					/* [8] explicit padding */ unsigned char _pad[8];
				} VolumeResendRequest;

			};

			// If extra data exists, it starts at (data + 8). If these 8 bytes are unused by the receiving function,
			// they are to be considered alignment padding.
			union {
				char   _data[8];
				size_t _dataLength;
			} Extra;

		};

	};

}