#include "NetworkStructures.h"

namespace Networking {
	
	bool ClientGUID::operator < (const ClientGUID& ID) const {
		return this->ID < ID.ID;
	}

	void formatGUIDAsString(char* str, Networking::ClientGUID guid)
	{
		sprintf(str, "%uh.%uh.%uh.%uh %uh.%uh.%uh.%uh", guid.BroadcastIP.Byte1, guid.BroadcastIP.Byte2,
														guid.BroadcastIP.Byte3, guid.BroadcastIP.Byte4,
														guid.LocalIP.Byte1,     guid.LocalIP.Byte2,
														guid.LocalIP.Byte3,     guid.LocalIP.Byte4);
	}
}