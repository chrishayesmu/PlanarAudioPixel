// -----------------------------------------------------------------
// This file contains global variables which are used across multiple
// of the networking files, as well as being usable from outside
// the networking library.
//
// Author: Chris Hayes
// -----------------------------------------------------------------

#include <map>
#include "NetworkStructures.h"

namespace Networking
{
	// A map between client GUIDs and the client information structures. Should be
	// used to store information about currently connected clients, as defined in
	// section 3.1 of the PAP document.
	extern std::map<ClientGUID, Client> ClientInformationTable;
}