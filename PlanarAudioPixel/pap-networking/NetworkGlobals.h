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

	// The port number on which to run network communications between the server and client.
	extern unsigned short NetworkPort;

	// The number of samples to buffer before initiating transport controls.
	extern unsigned int RequiredBufferedSamplesCount;

	// The number of sample to buffer while playing
	extern unsigned int ContinuousBufferCount;

	// Gets the current system time as a count of microseconds.
	time_t getMicroseconds();
}