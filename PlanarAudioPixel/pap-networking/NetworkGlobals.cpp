// Implementation file for NetworkGlobals.h
// Author: Chris Hayes

#include "NetworkGlobals.h"

#include <Windows.h>

namespace Networking
{
	std::map<ClientGUID, Client> ClientInformationTable;

	unsigned short NetworkPort = 32746;

	time_t getMicroseconds(){
		FILETIME time;
		GetSystemTimePreciseAsFileTime(&time);
		ULARGE_INTEGER lTime;
		lTime.LowPart = time.dwLowDateTime;
		lTime.HighPart = time.dwHighDateTime;
		return (lTime.QuadPart / 10);
	}
}