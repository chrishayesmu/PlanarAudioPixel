

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
}