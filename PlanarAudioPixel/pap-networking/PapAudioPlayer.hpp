#ifdef PLAYBACKCLIENT
#include <portaudio.h>
#include <sndfile.h>

#include "sf_virtual_wrapper.h"

class PapAudioPlayer
{
	friend int audioOutputCallback(const void* input, void* output, unsigned long frameCount, const PaStreamCallbackTimeInfo* timeInfo,
												PaStreamCallbackFlags statusFlags, void* userData);

	public:
		// Creates a new PapAudioPlayer object.
		//
		// @param length 
		//              The length of the audio file that will be played,
		//              in bytes. This should include the entire audio file.
		//              Do not strip the file header or other metadata.
		//
		// @param sampleSize
		//              The size of each sample, in bytes, (e.g. 1024).
		//              This number is not necessarily related to the concept
		//              of "samples" as an audio file would contain, but rather,
		//              it refers to the way the audio file is sampled as it is
		//              sent across the network.
		PapAudioPlayer(int length, int sampleSize);

		~PapAudioPlayer();

		// Buffers the data provided into the internal audio object.
		// You cannot specifically provide a sample ID that should be
		// used. Instead you must always buffer the data in order.
		//
		// @param data 
		//             Pointer to the beginning of data to copy.
		// @param numBytes 
		//             How many bytes to copy from data.
		void bufferAudioData(void* data, int numBytes);

		// Sets the playback volume for the given sample.
		//
		// @param sampleID
		//             The sample ID associated with this volume.
		// @param volume
		//             The volume to play the sample at, between 0.0f and 1.0f.
		void setVolumeData(int sampleID, float volume);

		// Sets the audio player up to begin playback. Should not be called
		// until at least 256 bytes of data have been buffered.
		//
		// @return True if setup was successful. If false, you should be
		//         able to retrieve some info through perror. In addition,
		//         error information will be printed to stderr.
		bool setup();

		// Returns true if the audio is currently playing. 
		bool isAudioPlaying() const;

		// Returns true if the end of the internal buffer has been reached.
		bool endOfBuffer() const;

		// Plays the audio player from the beginning if it has not been previously played,
		// or from the last playback position if it has been played and then paused.
		bool play();

		// Pauses the audio player.
		bool pause();

		// Retrieves the audio file info associated with the file being played.
		// Cannot be called before setup().
		SF_INFO getFileInfo() const;
	private:
		// A pointer that is used for the virtual audio file. 
		SNDFILE* audioFile;

		// A virtual IO object used by libsndfile.
		SF_VIRTUAL_IO virtualIO;

		// Struct containing information about the audio file being played.
		SF_INFO audioFileInfo;

		// An audio buffer object used by the virtual IO system used with libsndfile.
		AudioBufferData* audioBuffer; 

		// The volume buffer. Indices are sample IDs corresponding to each volume.
		float* volumeBuffer;

		// A stream used for audio playback through PortAudio.
		PaStream* audioStream;

		// Tracks whether audio is being played.
		bool isPlaying;

		// Tracks whether the audio player has been initialized.
		bool isInitialized;

		// The expected size of each sample, in bytes.
		int sampleSize;

		// Tracks which sample is next to be played.
		int nextSampleID;

		// Which byte ( in range [0, sampleSize) ) should be played next
		// for the current sample.
		int nextByteInSample;

		// ---------- Functions ----------
		PaError openPortAudioStream();
};
#endif
