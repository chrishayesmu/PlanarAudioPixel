#include "NetworkStructures.h"
#include "Socket.h"
#include <queue>
//Giancarlo

namespace Networking {

	///<summary>A set of PlaybackServer states.</summary>
	enum PlaybackServerStates {
		PlaybackServer_RUNNING = 1,
		PlaybackServer_STOPPED = 3
	};
	typedef PlaybackServerStates PlaybackServerState;

	///<summary>A set of PlaybackServer error codes for the return values of certain functions.</summary>
	enum PlaybackServerErrorCodes {
		PlaybackServer_OK = 0,
		PlaybackServer_INVALID = 1,
		PlaybackServer_FILE = 2,
		PlaybackServer_POINTER = 3
	};
	typedef PlaybackServerErrorCodes PlaybackServerErrorCode;

	///<summary>A class that handles networking and audio data for managing a set of speaker nodes.</summary>
	class PlaybackServer {
	private:

		/// Messages are processed in the order in which they are recieved.

		///<summary>Used to identify a control message.</summary>
		enum PlaybackServerRequestCodes {
			PlaybackServer_PLAY,
			PlaybackServer_PAUSE,
			PlaybackServer_STOP,
			PlaybackServer_BUFFER,
			PlaybackServer_NEW_TRACK //,
			//PlaybackServer_Seek,
			//PlaybackServer_Restart, etc.
		};
		typedef PlaybackServerRequestCodes PlaybackServerRequestCode;

		///<summary>Used to service particular control messages.</summary>
		union PlaybackServerRequestData {
			struct {
				char audioFilename[256];
				char positionFilename[256];
				void (*callback)(int audioErrorCode, int positionErrorCode);
			} newTrackInfo;
			void (*bufferingCallback)(float percentBuffered);
			///time_t seekTo;
		};

		///<summary>Used to send control messages to the server like Pause/Stop/Restart, etc.</summary>
		struct PlaybackServerRequest {
			PlaybackServerRequestCode controlCode;
			PlaybackServerRequestData controlData;
		};

		// Control message queue
		std::queue<PlaybackServerRequest> controlMessages;
		CRITICAL_SECTION controlMessagesCriticalSection;
		HANDLE controlMessagesResourceCount;

		// Main receiving socket
		Socket* recvSocket, * sendSocket;

		// Maintains the servers current state. Initial state is STOPPED.
		PlaybackServerState state;
		HANDLE serverMainThread, serverReceivingThread;

		// Track buffer object
		TrackBuffer tracks;

		// Used to ensure delivery of the initial buffering samples
		typedef std::map<sampleid_t, unsigned int>::iterator SampleID_UInt_I;
		std::map<sampleid_t, unsigned int> sampleReceivedCounts;
		std::map<sampleid_t, unsigned int> volumeReceivedCounts;

		//Construction/destruction
		PlaybackServer();
		~PlaybackServer();

		///<summary>Receives information from a client and stores it in the client information list.</summary>
		///<param name="data">The datagram data.</param>
		///<param name="dataSize">The number of bytes in the datagram.</param>
		void receiveClientConnection(char* data, int dataSize);

		///<summary>Responds to a delay request sent by a client.</summary>
		///<param name="clientID">The ID of the client to send the response to.</param>
		void sendDelayResponseMessage(ClientGUID clientID);

		///<summary>Responds to an audio data resend request.</summary>
		///<param name="data">The datagram data.</summary>
		///<param name="dataSize">The number of bytes in the datagram.</param>
		void resendAudio(char* data, int dataSize);

		///<summary>Responds to a volume data resend request.</summary>
		///<param name="data">The datagram data.</summary>
		///<param name="dataSize">The number of bytes in the datagram.</param>
		void resendVolume(char* data, int dataSize);

		///<summary>Updates the client information table for the client that sent the check in.</summary>
		///<param name="data">The datagram data.</summary>
		///<param name="dataSize">The number of bytes in the datagram.</param>
		void receiveClientCheckIn(char* data, int dataSize);

		///<summary>This function will be called in order to notify the server that one or more 
		/// clients have moved, join, or dropped out, and that the volume must therefore be recalculated.</summary>
		void clientPositionsChanged();

		///<summary>Adds the file names to a new track server request to be handled by serverMain().</summary>
		///<param name="audioFilename">The name of the audio file.</summary>
		///<param name="positionFilename">The name of the position information data file.</summary>
		void processAudioFiles(char* audioFilename, char* positionFilename);

		///<summary>Reads audio data from the file and fills an AudioBuffer.</summary>
		///<param name="filename">The name of the audio file.</param>
		///<param name="buffer">The buffer to fill.</param>
		///<returns>TODO: Integer return code specifying the result of the call.</returns>
		int readAudioDataFromFile(char* filename, AudioBuffer& buffer);

		///<summary>Reads position data from the file and fills a PositionBuffer.</summary>
		///<param name="filename">The name of the position data file.</param>
		///<param name="buffer">The buffer to fill.</param>
		///<returns>TODO: Integer return code specifying the result of the call.</returns>
		int readPositionDataFromFile(char* filename, PositionBuffer& buffer);

		//## Possibly add an RTT field to the Client structure and periodically send RTT tracking requests. Then use it 
		//## to send an accurate expected wait time to a Client for receiving all samples in a buffer range.
		///<summary>Broadcasts an audio sample to the client network.</summary>
		///<param name="sampleBuffer">The sample data.</param>
		///<param name="bufferIDStart">The ID of the first sample in the buffer range currently being delivered.</param>
		///<param name="bufferIDEnd">The ID of the last sample in the buffer range currently being delivered.</param>
		///<returns>TODO: Integer return code specifying the result of the call.</returns>
		int sendAudioSample(AudioSample sampleBuffer, sampleid_t bufferIDStart, sampleid_t bufferIDEnd);

		///<summary>Broadcasts volume information for the client network.</summary>
		///<param name="sampleID">The ID of the sample to which this bit of volume data applies.</param>
		///<param name="trackID">The ID of the track to which this bit of volume data applies.</param>
		///<param name="volumeDataBuffer">The raw data containing the volume information.</param>
		///<param name="bufferSize">The number of bytes of volume data.</param>
		///<returns>Integer return code specifying the result of the call.</returns>
		int sendVolumeData(sampleid_t sampleID, trackid_t trackID, char* volumeDataBuffer, int bufferSize);

		///<summary>Single entry point for all network communications. Reads the control byte and acts on it accordingly.</summary>
		///<param name="datagram">The datagram data.</param>
		///<param name="datagramSize">The size of the datagram.</param>
		void dispatchNetworkMessage(char* datagram, int datagramSize);
		
		///<summary>Handles network communications and hands off incoming packets to dispatchNetworkMessage().</summary>
		void serverReceive();
		///<summary>Multithreaded router function that calls serverReceieve().</summary>
		static DWORD __stdcall serverRouteReceive(void* server);

		///<summary>Handles server management and control messages.</summary>
		void serverMain();
		///<summary>Multithreaded router function that calls serverMain().</summary>
		static DWORD __stdcall serverRouteMain(void* server);

		///<summary>Queues up a request code to be handled by serverMain().</summary>
		///<param name="code">The request code.</param>
		void queueRequest(PlaybackServerRequestCode code);
		
		///<summary>Queues up a request code to be handled by serverMain().</summary>
		///<param name="code">The request code.</param>
		///<param name="requestData">Additional data required to complete the request.</param>
		void queueRequest(PlaybackServerRequestCode code, PlaybackServerRequestData requestData);

	public:

		///<summary>Attempts to create a PlaybackServer instance and returns an error code if it could not. fillServer is filled with NULL if creation fails.</summary>
		///<param name="fillServer">A reference to the PlaybackServer object to fill.</param>
		///<returns>A Networking::SocketErrorCode.</returns>
		static int Create(PlaybackServer** fillServer);
		
		///<summary>Attempts to start the PlaybackServer.</summary>
		///<returns>A PlaybackServerState code indicating the state of this call. If the state is not RUNNING, an error code is returned.</returns>
		int Start();

		///<summary>Attempts to start playback.</summary>
		///<returns>A PlaybackErrorCode indicating the result of this call.</returns>
		int Play();

	};


};