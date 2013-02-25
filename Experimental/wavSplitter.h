/*
This is still a work in progress. I'm stopping where I'm at now, and waiting to see what Chris wants to do with sound files.
I'm hoping he finds a better method than what is below.
*/
#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<vector>
#include<stdint.h>

#include<iostream>
using namespace std;

#define STARTNUM 0
#define FILENAMESIZE 20

/*
This is the standard wav file header format.
I found this datastructure online.
I found shitty parsing code online as well, this file is "mostly" corrected adapation of that.
*/
typedef struct WAVHEADER 
	{
	char RIFF[4];
	uint32_t totalsizem8;
	char WAV[4];
	char fmt[4];
	uint32_t fmtsize;
	int16_t audiofmt;
	int16_t nchannels;
	uint32_t samplerate;
	uint32_t byterate;
	int16_t blockalign;
	int16_t bitspersample;
	char data[4];
	uint32_t restsize;
	}WAVHEADER;
/*
This will be used to store wav files entirely in memory for wavSplitter 2.0
We can then further rework the format so that they are in contiguous memory for packets
As it stands, you could still use it to load a wav file from file into memory - see audioplayback.cpp
*/
typedef struct WAVFILE
	{
	WAVHEADER header;
	char *data;
	}WAVFILE;

std::vector<WAVFILE*> gWavFiles;

int splitWavFile( char *aFileName, int aTimeInterval );

int splitWavFile( char *aFileName, int aTimeInterval )
	{
	FILE *mFptr, *mOptr; //Pointers could be named better
	WAVHEADER *mHeader;
	char *mBuffer;
	long mDataSetLength, mRate, mSize, mLastSplit, mNextSplit = 0, mNextCount, mNumberOfNewWavFiles;
	char mOutFileName[FILENAMESIZE];
	double mTime;
	
	mFptr = fopen( aFileName, "r" );
	mHeader = ( WAVHEADER* )malloc( sizeof( WAVHEADER ) );
	fread( mHeader, sizeof( WAVHEADER ), 1L, mFptr );
	/*	
	cout << "\nBegin header info: "<< aFileName << endl;
	cout << "RIFF Header: " << mHeader->RIFF << endl;
	cout << "RIFF Chunk Size: " << mHeader->totalsizem8 << endl;
	cout << "WAV Header: " << mHeader->WAV << endl;
	cout << "FMT header: " << mHeader->fmt << endl;
	cout << "Size of the fmt chunk: " << mHeader->fmtsize << endl;
	cout << "Audio format 1=PCM: " << mHeader->audiofmt << endl;
	cout << "Number of channels 1=Mono 2=Sterio: " << mHeader->nchannels << endl;
	cout << "Sampling Frequency in Hz: " << mHeader->samplerate << endl;
	cout << "bytes per second: " << mHeader->byterate << endl;
	cout << "2=16-bit mono, 4=16-bit stereo: " << mHeader->blockalign << endl;
	cout << "Number of bits per sample: " << mHeader->bitspersample << endl;
	cout << "data  string: " << mHeader->data << endl;
	cout << "Sampled data length: " << mHeader->restsize << endl;
	cout << "End Header info\n" << endl;
	*/
	mTime = (double)mHeader->totalsizem8 / (double)( mHeader->samplerate * mHeader->nchannels * mHeader->bitspersample / 8.0 );
	//cout << "File play length in seconds: " << mTime << endl;
	
	mDataSetLength = mHeader->blockalign;
	//cout << "DatsetLength = " << mDataSetLength << endl;
	
	/*
	There is math to determine what this should be, but I was trying to work on parsing nonstandard files so I didn't bother finding it
	118 is just the difference in my normal wav file that I was testing against
	*/
	if( mHeader->restsize !=  mHeader->totalsizem8 - 118 )
		{
		cout << "\nFile is nonstandard .wav format" << endl;
		/*
		This is where we would strip out all nonstandard header info
		If we don't want to do it ourselves
		http://www.lightlink.com/tjweber/StripWav/StripWav.html
		is the link for a stripper program
		*/
		fclose( mFptr );
		free( mHeader );
		return 0;
		}
	
	
	mRate = mHeader->byterate / mDataSetLength;
	//cout << "Rate = " << mRate << endl;
	
	mSize = mHeader->restsize / mHeader->blockalign;
	//cout << "Size = " << mSize << endl;
	
	mBuffer = (char*)malloc( mDataSetLength );
	
	mNumberOfNewWavFiles = mTime / aTimeInterval;
	//cout << "\nNumberOfNewWavFiles = " << mNumberOfNewWavFiles << endl;
	
	if( mNumberOfNewWavFiles == 0 )
		{
		mNumberOfNewWavFiles = 1;
		}
		
	for( int i = 1; i <= mNumberOfNewWavFiles; i++ )
		{
		mLastSplit = mNextSplit;
		if( i == mNumberOfNewWavFiles )
			{
			mNextSplit = mSize;
			//cout << "\nSize: " << mSize << endl;
			}
		else
			{
			mNextSplit = mLastSplit + aTimeInterval * mRate;
			}
		//cout << "Next Split: " << mNextSplit << endl;
		
		mNextCount = mNextSplit - mLastSplit;
		//cout << "Next Count: " << mNextCount << endl;
		
		mHeader->restsize = mNextCount * mDataSetLength;
		//cout << "Rest size: " << mHeader->restsize << endl;
		
		mHeader->totalsizem8 = mHeader->restsize + 8L + sizeof( WAVHEADER );
		snprintf( mOutFileName, FILENAMESIZE, "%d.wav", i );
		mOptr = fopen( mOutFileName, "w" );
		fwrite( mHeader, sizeof( WAVHEADER ), 1L, mOptr );
		for( int j = 0; j < mNextCount; j++ )
			{
			fread( mBuffer, mDataSetLength, 1L, mFptr );
			fwrite( mBuffer, mDataSetLength, 1L, mOptr );
			}
		fclose( mOptr );
		}
		
	free( mBuffer );
	free( mHeader );
	fclose( mFptr );
	return mNumberOfNewWavFiles;
	}