#pragma once

namespace IO {

	// A structure containing the data and information regarding an audio sample.
	struct AudioData 
	{
		// The raw data from the audio file for this sample.
		char Data[1468];

		// The number of bytes in the data for this IO. No larger than the defined audio chunk size.
		unsigned int DataLength;
	};

};