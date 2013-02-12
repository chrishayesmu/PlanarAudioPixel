#include <time.h>
#include <stdint.h>

namespace PlanarAudioPixel {

	// A structure for representing IP addresses, as either a 4-byte integer
	// or as 4 separate bytes, for convenience.
	public value class IP_Address
	{
	public:
		// The 4-byte IP address, stored as an integer.
		uint32_t RawIP;

	};

	// A struct for representing positions, for compactness and in case we need
	// to easily increase the precision of the position.
	public value class PositionInfo
	{
	public:
		float x;
		float y;
	};

	// A structure for representing a client's globally unique ID (GUID). This 
	// structure is mostly used for creating a mapping to clients for the
	// Client Information Table.
	public value class ClientGUID
	{
	public:
		IP_Address BroadcastIP;
		IP_Address LocalIP;
	};

	// A structure for representing a client's position and networking information.
	// Defined in section 3.1 of the PAP document.
	public value class Client
	{
	public:
		//The client ID
		ClientGUID ClientID;
		
		// The client's position in the grid, stored as an offset from the top-left corner of the grid,
		// which has position (0, 0).
		PositionInfo Offset;

		// The 8-byte Unix timestamp of when the client last checked in with the server.
		time_t LastCheckInTime;
	};

}