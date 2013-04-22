// Implementation file for NetworkGlobals.h
// Author: Chris Hayes

#include "NetworkGlobals.h"

#include <Windows.h>
#include <time.h>

namespace Networking
{
	//std::map<ClientGUID, Client> ClientInformationTable;

	unsigned short NetworkPort = 32746;

	unsigned int RequiredBufferedSamplesCount = 10;

	unsigned int ContinuousBufferCount = 100;

	time_t ClientReceivedPacketTimeout = 200000;

	time_t getMicroseconds(){
		FILETIME time;

		// TODO: See about replacing this with less precise version
		GetSystemTimePreciseAsFileTime(&time);
		ULARGE_INTEGER lTime;
		lTime.LowPart = time.dwLowDateTime;
		lTime.HighPart = time.dwHighDateTime;
		return (lTime.QuadPart / 10);
	}
	
	// Performs a spin wait for the specified number of microseconds. Exact microsecond accuracy is not super important here. It is, however, at the very least accurate to the millisecond.
	void busyWait(time_t us){
		us += getMicroseconds();
		while (getMicroseconds() < us);
	}
}