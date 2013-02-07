// Implementation file for NetworkGlobals.h
// Author: Chris Hayes

#include "NetworkGlobals.h"

namespace Networking
{
	std::map<ClientGUID, Client> ClientInformationTable;

	unsigned short NetworkPort = 32746;
}