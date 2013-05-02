#ifdef PLAYBACKCLIENT
/*
* This file defines a function for retrieving an SF_VIRTUAL_IO struct
* that is configured for buffered data. It also declares a struct which
* is to be used in conjunction with the SF_VIRTUAL_IO retrieved in this way.
*
* Users of this file should follow these steps:
*
* 1) Determine the full length of the file, in bytes, that will eventually be
*    stored in the buffer. This includes any metadata stored within the file.
*
* 2) Call get_buffered_sf_virtual_io and save the return value.
*
* 3) Use add_to_buffer to store at least 1024 bytes of data in the buffer.
*
* 4) Call sf_open_virtual. Pass the return value from (1) and the AudioBufferData
*    object that was used in (1).
*
* 5) The object is set up and ready to use with sf_read or other functions. You
*    should add data to the buffer periodically with add_to_buffer.
*
* VERY IMPORTANT: The struct (AudioBufferData) declared here cannot be accessed directly
* and can only be accessed through a pointer (similar to type FILE*). Whenever it
* is created via get_buffered_sf_virtual_io, the pointer created MUST be the
* pointer which is passed to sf_open_virtual in place of the "void* user_data"
* parameter. All of the streaming functions rely on this behavior. Accordingly,
* the memory must never be freed until the stream is closed and the virtual SNDFILE*
* from sf_open_virtual is closed. If you pass some other pointer to sf_open_virtual, or
* free the AudioBufferData* pointer prematurely, or copy some other data into the AudioBufferData
* memory, undefined (and very bad) behavior will result.
*
* NOTE: libsndfile provides support for seeking in sound files. This is supported to a
* limited extent in buffered data; it is possible to seek backwards, but not forward beyond
* what is currently in the buffer.
*
* libsndfile also supports writing to sound files. This functionality is disabled for
* buffered data; calls to sf_write will have no effect.
*/

#include <sndfile.h>

typedef struct audioBufferData AudioBufferData;

// Creates an SF_VIRTUAL_IO object and simultaneously initializes an AudioBufferData object.
//
// INPUT: audioBuffer - A pointer to an AudioBufferData to be filled.
//        length      - The length of the file that will be stored in the buffer, in bytes.
SF_VIRTUAL_IO get_buffered_sf_virtual_io(AudioBufferData** audioBuffer, int length);

// Adds data to the audio buffer. 
int add_to_buffer(AudioBufferData* audioBuffer, void* data, int numBytes);

// Determines if the audio buffer has reached its ultimate end or is just temporarily empty.
int end_of_buffer(AudioBufferData* audioBuffer);

void delete_buffer(AudioBufferData* audioBuffer);
#endif
