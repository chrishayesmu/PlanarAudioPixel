#ifdef PLAYBACKCLIENT
/*
* Definition file for sf_virtual_wrapper.h
*
* Current implementation requires server to send length of the
* audio file as the first 4 bytes of the stream.
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#include <stdlib.h>
#include <string.h>

// stdio is only needed for debugging
#ifdef _STREAM_DEBUG
#include <stdio.h>
#endif

#include "sf_virtual_wrapper.h"

// Functions required as part of the SF_VIRTUAL_IO struct.
// Details on their form can be found at:
// http://www.mega-nerd.com/libsndfile/api.html#open_virtual
sf_count_t streaming_sf_get_filelen(void* data);
sf_count_t streaming_sf_seek(sf_count_t offset, int whence, void* data);
sf_count_t streaming_sf_read(void* ptr, sf_count_t count, void* data);
sf_count_t streaming_sf_write(const void* ptr, sf_count_t count, void* data);
sf_count_t streaming_sf_tell(void* data);

typedef struct audioBufferData 
{
	int length;            // The total number of bytes that will eventually be buffered.
	int nextByteToRead;    // The next byte to be read from the buffer.
	int nextByteToBuffer;  // The next byte which will be buffered.
	char* buffer;          // A pointer to the audio data. Allocated once, when the structure is created.
} AudioBufferData;

// The initial size of the audio buffer.
#ifndef _BUFFER_INIT_SIZE
#define _BUFFER_INIT_SIZE 8192
#endif

// The amount to multiply the buffer size by when resizing.
#ifndef _BUFFER_RESIZE_FACTOR
#define _BUFFER_RESIZE_FACTOR 2
#endif

// Creates a struct of type SF_VIRTUAL_IO and initializes the AudioBufferData* provided.
// A socket handle must be provided that can be accessed by the SF_VIRTUAL_IO methods later.
//
// INPUT:  AudioBufferData   - A pointer to a StreamData* to be filled with a StreamData struct.
//         socketHandle - The handle of a socket which is already opened and connected.
//
// OUTPUT: AudioBufferData is filled with a valid StreamData*.
//         A usable SF_VIRTUAL_IO struct is returned for use with sf_open_virtual.
SF_VIRTUAL_IO get_buffered_sf_virtual_io(AudioBufferData** audioBuffer, int length)
{
	// Set up SF_VIRTUAL_IO struct
	SF_VIRTUAL_IO virtIO;

	virtIO.get_filelen = streaming_sf_get_filelen;
	virtIO.seek = streaming_sf_seek;
	virtIO.read = streaming_sf_read;
	virtIO.write = streaming_sf_write;
	virtIO.tell = streaming_sf_tell;

	// Set up internal AudioBufferData struct
	*audioBuffer = (AudioBufferData*) malloc(sizeof(AudioBufferData));
	(*audioBuffer)->buffer = (char*) malloc(length);
	(*audioBuffer)->nextByteToBuffer = 0;
	(*audioBuffer)->nextByteToRead = 0;
	(*audioBuffer)->length = length;

	return virtIO;
}

// Deletes the internal audio buffer.
void delete_buffer(AudioBufferData* audioBuffer)
{
	free(audioBuffer->buffer);
}

// Returns the number of bytes in the stream.
sf_count_t streaming_sf_get_filelen(void* data)
{
	AudioBufferData* audioBuffer = (AudioBufferData*) data;

	return audioBuffer->length;
}

// Seeks around in the data stream. Seeking forward in the stream is
// only possible if it ends in buffered data. Seeking backwards is always
// possible due to buffering.
sf_count_t streaming_sf_seek(sf_count_t offset, int whence, void* data)
{
	AudioBufferData* audioBuffer = (AudioBufferData*) data;

	// If stream is already in invalid state, don't attempt to seek
	if (audioBuffer->nextByteToRead >= audioBuffer->nextByteToBuffer) 
	{
		return audioBuffer->nextByteToBuffer - 1;
	}

	int desiredPosition = -1;
	int lastPossiblePosition = audioBuffer->nextByteToBuffer - 1;

	switch (whence)
	{
		case SEEK_SET: // Seek relative to the start of buffer
			if (offset <= lastPossiblePosition) // Ensure new offset is between current position and EOF
				desiredPosition = offset;
			break;
		case SEEK_CUR: // Seek relative to current position in stream
			if (offset + audioBuffer->nextByteToRead <= lastPossiblePosition && offset + audioBuffer->nextByteToRead >= 0)
				desiredPosition = audioBuffer->nextByteToRead + offset;
			break;
		case SEEK_END: // Seek relative to the end of the stream
			if (offset <= 0 && audioBuffer->nextByteToBuffer - 1 + offset <= lastPossiblePosition) // Make sure we aren't moving forwards
				//desiredPosition = audioBuffer->length - 1 + offset;
				desiredPosition = audioBuffer->nextByteToBuffer - 1 + offset;
			break;
		default: // Invalid argument; stream remains in same state of validity
			return -1;
	}

	if (desiredPosition >= 0)
		audioBuffer->nextByteToRead = desiredPosition;

	return audioBuffer->nextByteToRead;
}

// Reading data copies from the stream into the output buffer. If the end
// of the stream is reached then the stream will be left in an invalid state
// after reading as many bytes as possible.
sf_count_t streaming_sf_read(void* out, sf_count_t count, void* data)
{
	AudioBufferData* audioBuffer = (AudioBufferData*) data;

	// Check for valid state of stream
	if (audioBuffer->nextByteToRead >= audioBuffer->nextByteToBuffer)
		return 0;

	int bytesAvailable = audioBuffer->nextByteToBuffer - audioBuffer->nextByteToRead;

	if (count > bytesAvailable)
		count = bytesAvailable;

	memcpy(out, &audioBuffer->buffer[audioBuffer->nextByteToRead], count);
	audioBuffer->nextByteToRead += count;

	return count;
}

// Writing is unsupported and always fails. The stream is left in the same state
// as before the write.
sf_count_t streaming_sf_write(const void* ptr, sf_count_t count, void* data)
{
	return 0;
}

// Returns current position in stream. The stream is left in the same state as
// before the tell.
sf_count_t streaming_sf_tell(void* data)
{
	AudioBufferData* audioBuffer = (AudioBufferData*) data;
	return audioBuffer->nextByteToRead;
}

int add_to_buffer(AudioBufferData* audioBuffer, void* data, int numBytes)
{
	// Check how many bytes are left in buffer
	int bytesRemaining = audioBuffer->length - audioBuffer->nextByteToBuffer;
	if (numBytes > bytesRemaining)
		numBytes = bytesRemaining;

	memcpy(&audioBuffer->buffer[audioBuffer->nextByteToBuffer], data, numBytes);
	audioBuffer->nextByteToBuffer += numBytes;

	return numBytes;
}

int end_of_buffer(AudioBufferData* audioBuffer)
{
	return audioBuffer->nextByteToRead == audioBuffer->length;
}
#endif
