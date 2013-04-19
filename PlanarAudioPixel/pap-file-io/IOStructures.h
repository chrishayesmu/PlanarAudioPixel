#pragma once
#include "../pap-networking/NetworkGlobals.h"

namespace IO {

	// A structure containing the data and information regarding an audio sample.
	struct AudioData 
	{
		// The raw data from the audio file for this sample.
		char Data[Networking::SampleSize];

		// The number of bytes in the data for this IO. No larger than the defined audio chunk size.
		unsigned int DataLength;
	};

};