#ifdef RASPBERRY_PI
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "PapAudioPlayer.hpp"

int audioOutputCallback(const void* input, void* output, unsigned long frameCount, const PaStreamCallbackTimeInfo* timeInfo,
						PaStreamCallbackFlags statusFlags, void* userData);

PapAudioPlayer::PapAudioPlayer(int length, int sampleSize) : audioFile(NULL), audioStream(NULL), isPlaying(false), isInitialized(false), sampleSize(sampleSize), nextSampleID(0), nextByteInSample(0)
{
	// Set up virtual IO
	this->virtualIO = get_buffered_sf_virtual_io(&this->audioBuffer, length);

	// Allocate volumeBuffer with room for one extra sample in case of rounding
	int numSamples = length / sampleSize;
	this->volumeBuffer = (float*) malloc( (numSamples + 1) * sizeof(float) );
}

PapAudioPlayer::~PapAudioPlayer()
{
	Pa_StopStream(this->audioStream);
	Pa_Terminate();
	delete_buffer(this->audioBuffer);

	// The following segfaults right now and I'm not sure how relevant it
	// is with how I'm doing IO anyway, so I'm leaving it out
/*	if (sf_close(this->audioFile) != 0)
		perror("Failed when closing file: ");*/
}

bool PapAudioPlayer::setup()
{
	if (this->isInitialized)
		return false;

	// Open virtual file
	this->audioFile = sf_open_virtual(&this->virtualIO, SFM_READ, &audioFileInfo, this->audioBuffer);

	// Initialize PortAudio
	PaError audioError = Pa_Initialize();
	if (audioError != paNoError)
	{
		fprintf(stderr, "Error initializing PortAudio: %s\n", Pa_GetErrorText(audioError));
		return false;
	}

	// Create a PortAudio stream
	audioError = openPortAudioStream();
	if (audioError != paNoError)
	{
		fprintf(stderr, "Error opening PortAudio stream: %s\n", Pa_GetErrorText(audioError));
		return false;
	}

	this->isInitialized = true;

	return true;
}

void PapAudioPlayer::bufferAudioData(void* data, int numBytes)
{
	add_to_buffer(this->audioBuffer, data, numBytes);
}

void PapAudioPlayer::setVolumeData(int sampleID, float volume)
{
	this->volumeBuffer[sampleID] = volume;
}

bool PapAudioPlayer::play()
{
	PaError audioError = Pa_StartStream(this->audioStream);
	if (audioError != paNoError)
	{
		fprintf(stderr, "Error starting PortAudio stream: %s\n", Pa_GetErrorText(audioError));
		return false;
	}

	this->isPlaying = true;

	return true;
}

SF_INFO PapAudioPlayer::getFileInfo() const 
{
	return this->audioFileInfo;
}

bool PapAudioPlayer::isAudioPlaying() const
{
	return isPlaying;
}

// ---------- Private functions ----------

PaError PapAudioPlayer::openPortAudioStream()
{
	return Pa_OpenDefaultStream(&this->audioStream, 			// stream object to be filled
								0, 							    // number of input channels; none for our case
								this->audioFileInfo.channels, 	// number of output channels
								paFloat32, 				        // audio format; we're using 32-bit floats
								this->audioFileInfo.samplerate, // sample rate of playback
								paFramesPerBufferUnspecified,   // number of frames per buffer; we'll let PortAudio choose for us
								audioOutputCallback, 	        // callback function that will be called when audio data is needed
								this	     					// some data that will be sent to the callback as context
							   );
}

// ---------- Friend functions ----------

int audioOutputCallback(const void* input, void* output, unsigned long frameCount, const PaStreamCallbackTimeInfo* timeInfo,
										PaStreamCallbackFlags statusFlags, void* userData)
{
	// The audio callback needs to run in real time. That means no IO calls
	// (except through libsndfile), no system calls, and nothing requiring
	// a switch to kernel mode. Some very minor optimizations have been
	// applied to speed this function up.

	PapAudioPlayer* player = (PapAudioPlayer*) userData;
	float* out = (float*) output;
	float tempBuffer[player->sampleSize];

	int numRequestedBytes = 2 * frameCount * player->audioFileInfo.channels;

	while (numRequestedBytes > 0)
	{
		int bytesLeftInSample = player->sampleSize - player->nextByteInSample;
		int bytesToRead = bytesLeftInSample < numRequestedBytes ? bytesLeftInSample : numRequestedBytes; 
		
		int framesToRead = bytesToRead / ( 2 * player->audioFileInfo.channels );

		// I don't know why this is multiplied by 4, but it works
		int bytesRead = 4 * sf_readf_float(player->audioFile, tempBuffer, framesToRead) * player->audioFileInfo.channels;

		// If no bytes read, we've reached EOF, so end playback completely
		if (bytesRead == 0)
		{
			player->isPlaying = false;
			return paComplete;
		}

		// Go through the data and adjust the volume
		float vol = player->volumeBuffer[ player->nextSampleID ];
		for (int i = 0; i < bytesRead / 2; i++)
		{
			tempBuffer[i] *= vol;
		}

		// Copy the adjusted data to the output stream; advance stream to match
		memcpy(out, tempBuffer, bytesRead);
		out += bytesRead;

		// Update iteration variables and class members
		numRequestedBytes -= bytesRead;
		player->nextByteInSample += bytesRead;

		// Loop back around if end of sample is reached
		if (player->nextByteInSample >= player->sampleSize)
		{
			player->nextByteInSample = 0;
			player->nextSampleID++;
		}
	}

	return 0;
}
#endif