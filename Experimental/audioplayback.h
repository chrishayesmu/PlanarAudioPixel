/*
This file is how I was playing .wav files in c++ code. It is terrible.
This file has been almost completely ripped off from:
http://paste2.org/p/2708324

Using alsa library is the best I found, but it's shit. Need to find better c++ tools.
This will work, but there has to be an easier way.


compile instructions
g++ audioplayback.cpp -lasound
g++ audioplayback.cpp -lasound -o c++Audio

To play a sound file from command line
aplay -vv /path/to/file.wav

Here are some code snippets to explore. This is how one person suggest parsing audio into raw pcm data.
I gave up this avenue of exploration in favor of generalized .wav parsing. See wavSplitter.h

typedef char[180] audio_data_chunk; 
audio_data_chunk adata; 
snd_pcm_read(handle, adata, sizeof(adata); 
snd_pcm_writei(handle, adata, sizeof(adata);

These are just snippets. Whats missing in between is a lot of pcm work to change handles. You can get an idea of what
that involves from the code below.
*/

#include <alsa/asoundlib.h>
#include <cmath>
#include <cstdio>
#include <stdint.h>
#include <vector>
#include <iostream>

snd_pcm_t *pcm_handle;
using std::vector;

typedef struct  wavHeader
{
    char                RIFF[4];        /* RIFF Header      */ //Magic header
    uint32_t       		ChunkSize;      /* RIFF Chunk Size  */
    char                WAVE[4];        /* WAVE Header      */
    char                fmt[4];         /* FMT header       */
    uint32_t       		Subchunk1Size;  /* Size of the fmt chunk*/
    int16_t     		AudioFormat;    /* Audio format 1=PCM,6=mulaw,7=alaw, 257=IBM Mu-Law, 258=IBM A-Law, 259=ADPCM */
    int16_t     		NumOfChannels;  /* Number of channels 1=Mono 2=Sterio                   */
    uint32_t	        SamplesPerSec;  /* Sampling Frequency in Hz                             */
    uint32_t            bytesPerSec;    /* bytes per second */
    int16_t     		blockAlign;     /* 2=16-bit mono, 4=16-bit stereo */
    int16_t     		bitsPerSample;  /* Number of bits per sample      */
    char                Subchunk2ID[4]; /* "data"  string   */
    uint32_t       		Subchunk2Size;  /* Sampled data length    */
    char data[1];
} wavHeader;

vector<wavHeader*> loaded_sounds;

bool sound_add(const char *filename) {
	FILE *file;
	
	if ((file = fopen(filename,"rb")) == NULL)
	printf("ERROR: Can't open: %s", filename);
	
	fseek(file, 0, SEEK_END); 
	size_t flen = ftell(file); 
	fseek(file, 0, SEEK_SET); 
	char *wavfile = new char[flen]; 
	fread(wavfile, 1, flen, file);
	wavHeader *wavhead = (wavHeader*)wavfile;
	
	loaded_sounds.push_back(wavhead);
	return 0;
}

bool sound_play() {

unsigned int pcm;
int rate, channels, seconds;
snd_pcm_t *pcm_handle;
snd_pcm_hw_params_t *params;

/* Open the PCM device in playback mode */
if (pcm = snd_pcm_open(&pcm_handle, "default",SND_PCM_STREAM_PLAYBACK, 0) < 0)
printf("ERROR: Can't open \"%s\" PCM device. %s\n","default", snd_strerror(pcm));

/* Allocate parameters object and fill it with default values*/
snd_pcm_hw_params_alloca(&params);
snd_pcm_hw_params_any(pcm_handle, params);

/* Set parameters */
if (pcm = snd_pcm_hw_params_set_access(pcm_handle, params,SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
printf("ERROR: Can't set interleaved mode. %s\n", snd_strerror(pcm));

if (pcm = snd_pcm_hw_params_set_format(pcm_handle, params,SND_PCM_FORMAT_S16_LE) < 0)
printf("ERROR: Can't set format. %s\n", snd_strerror(pcm));

if (pcm = snd_pcm_hw_params_set_channels(pcm_handle, params, 1) < 0)
printf("ERROR: Can't set channels number. %s\n", snd_strerror(pcm));

snd_pcm_hw_params_set_rate_near(pcm_handle, params, &loaded_sounds[0]->SamplesPerSec, NULL);

/* Write parameters */
if (pcm = snd_pcm_hw_params(pcm_handle, params) < 0)

printf("ERROR: Can't set harware parameters. %s\n", snd_strerror(pcm));

/*
//My stuff
int testSize = loaded_sounds[0]->Subchunk2Size / loaded_sounds[0]->bitsPerSample;
char soundArray[8][testSize];
for( int i = 0; i < 8; i++ )
	{
	for( int j = 0; j < testSize; j++ )
		{
		soundArray[i][j] = loaded_sounds[0]->data[ i * testSize + j ];
		}
	}
for( int i = 0; i < 8; i++ )
	{
	snd_pcm_writei( pcm_handle, soundArray[i], testSize );
	}
snd_pcm_prepare(pcm_handle);
//My stuff ends
*/
	std::cout << "\nSubchunk2Size: " << loaded_sounds[0]->Subchunk2Size << std::endl;
	std::cout << "\n" << loaded_sounds[0]->Subchunk2Size << " " << loaded_sounds[0]->bitsPerSample << std::endl;
	std::cout << "Pcm frames to be written: " << loaded_sounds[0]->Subchunk2Size / loaded_sounds[0]->bitsPerSample * 8 << std::endl;
	snd_pcm_writei(pcm_handle,loaded_sounds[0]->data, loaded_sounds[0]->Subchunk2Size / loaded_sounds[0]->bitsPerSample * 8);
	snd_pcm_prepare(pcm_handle);
	return 0;
}

int main() {
	sound_add("../Beer_Beer_Beer.wav");
	sound_play();
	return 0;
}