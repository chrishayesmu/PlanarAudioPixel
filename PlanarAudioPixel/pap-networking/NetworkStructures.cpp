#include "NetworkStructures.h"

namespace Networking {

	void formatGUIDAsString(char* str, Networking::ClientGUID guid)
	{
		IP_Address ip;
		ip.RawIP = guid;

		sprintf(str, "%u.%u.%u.%u", ip.Byte1, ip.Byte2, ip.Byte3, ip.Byte4);
	}
}