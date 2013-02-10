#include "NetworkStructures.h"

namespace Networking {
	
	bool ClientGUID::operator < (const ClientGUID& ID) const {
		return this->ID < ID.ID;
	}

}