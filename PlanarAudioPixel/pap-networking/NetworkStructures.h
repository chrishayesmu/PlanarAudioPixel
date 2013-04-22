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
#include <stdint.h>
#include <time.h>
#include "sockets.h"

namespace Networking
{
	
	// A structure for representing IP addresses, as either a 4-byte integer
	// or as 4 separate bytes, for convenience.
	union IP_Address
	{
		// The 4-byte IP address, stored as an integer.
		uint32_t RawIP;

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
	typedef uint32_t ClientGUID;

	/// <summary>Formats a ClientGUID object into a string. The string will be "x.x.x.x x.x.x.x", where the first
	/// quartet is the broadcast IP and the second quartet is the local IP.</summary>
	/// <param name='str'>A string to fill with the ClientGUID. Should contain room for at least 32 bytes.</param>
	/// <param name='guid'>The ClientGUID to use when filling the string.</param>
	void formatGUIDAsString(char* str, Networking::ClientGUID guid);

	// A structure for representing a client's position and networking information.
	// Defined in section 3.1 of the PAP document.
	struct Client
	{
		ClientGUID ClientID;

		// The client's position in the grid, stored as an offset from the top-left corner of the grid,
		// which has position (0, 0).
		PositionInfo Offset;

		// The associated TCP socket
		int s;
	};

	typedef uint32_t trackid_t;
	typedef uint32_t sampleid_t;
	typedef uint64_t requestid_t;

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
	typedef std::map<ClientGUID, float>::iterator VolumeInfoIterator;
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

		// The time, in microseconds, when this track BEGAN playback.
		time_t playbackOriginOffset;

		// The time, in microseconds, left in this track.
		time_t playbackRemainingTime;

		// The length, in microseconds, of this track.
		time_t trackLength;

		// A buffer for the position data for each sample.
		PositionBuffer positionData;
		
		// A buffer for the raw audio data for this track.
		AudioBuffer audioSamples;

		// A buffer for the volume data for this track.
		VolumeBuffer volumeData;

		// The length of the file, in bytes
		uint32_t fileSize;

	};

	// A typedef for a TrackBuffer object, which is a map between track IDs and
	// TrackInfo objects
	typedef std::map<trackid_t, TrackInfo> TrackBuffer;

	
	namespace PacketStructures {

		// [16] A struct for specifying the volume of two clients for a volume data message.
		//		Two clients are packed into this struct to align on an 8-byte boundary and
		//		save 4 bytes per client while still maintaining a simple programming paradigm.
	/*
	2>  class ClientVolume	size(16):
	2>  	+---
	2>   0	| ClientGUID clientID_1
	2>   4	| float		 clientVolume_1
	2>   8	| float		 clientVolume_2
	2>  12	| ClientGUID clientID_2
	2>  	+---
	*/
		struct ClientVolume 
		{
			/* [4] */ ClientGUID clientID_1;
			/* [4] */ float  clientVolume_1;
			/* [4] */ float  clientVolume_2;
			/* [4] */ ClientGUID clientID_2;
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
					/* [4] */ ClientGUID clientID;
								uint32_t _client_pad;
					/* [8] */ PositionInfo position;
				} ClientConnection;

				// [16] Client to Server - Check-in
				struct {
					// The ID and position of the client
					/* [8] */ ClientGUID clientID;
								uint32_t _client_pad;
					/* [8] */ PositionInfo position;
				} ClientCheckIn;
				
				// [16] Server to Clients - Audio sample header
				struct {
					/* [4] */ trackid_t TrackID;
					/* [4] */ sampleid_t SampleID;
					/* [4] */ uint32_t fileSize;
					/* [4] */ uint32_t _unused;
				} AudioSample;
				
				// [16] Server to Clients - Volume sample header
				struct {
					/* [4] */ trackid_t TrackID;
					/* [4] */ sampleid_t SampleID;
					/* [4] */ float volume;
					/* [4] */ sampleid_t _unused;
				} VolumeSample;

				// [16] Transport controls
				union {
					//Server to Client - Control
					struct {
						/* [8] */ uint64_t timeOffset;
						/* [8] */ requestid_t requestID;
					};
					//Client to Server - Acknowledgement
					struct {
						/* [8] */ ClientGUID clientID;
								uint32_t _client_pad;
						#ifndef __GNUC__
						/* [8] */ requestid_t requestID;
						#else
						/* [8] */ requestid_t clientRequestID;
						#endif
					};
				} TransportControl;

				// [16] Server to Client - Disconnect message
				struct {
					/* [8] */ ClientGUID clientID;
					/* [8] explicit padding */ unsigned char _pad[12];
				} DisconnectNotification;

			};

			// If extra data exists, it starts at (data + 8). If these 8 bytes are unused by the receiving function,
			// they are to be considered alignment padding.
			union {
				char   _data[8];
				int64_t _dataLength;
				int64_t _extra;
			} Extra;

		};

	};

}