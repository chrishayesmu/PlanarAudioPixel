#include "NetworkStructures.h"
#include "Socket.h"
#include <queue>
//Giancarlo

namespace Networking {

	///<summary>A set of PlaybackServer states.</summary>
	enum PlaybackServerStates {
		PlaybackServer_RUNNING = 1,
		PlaybackServer_PAUSED = 2,
		PlaybackServer_STOPPED = 3
	};
	typedef PlaybackServerStates PlaybackServerState;

	///<summary>A set of PlaybackServer error codes for the return values of certain functions.</summary>
	enum PlaybackServerErrorCodes {
		PlaybackServer_OK = 0,
		PlaybackServer_INVALID = 1
	};
	typedef PlaybackServerErrorCodes PlaybackServerErrorCode;

	///<summary>A class that handles networking and audio data for managing a set of speaker nodes.</summary>
	class PlaybackServer {
	private:

		/// Messages are processed in the order in which they are recieved.

		///<summary>Used to identify a control message.</summary>
		enum PlaybackServerRequestCodes {
			PlaybackServer_PAUSE,
			PlaybackServer_STOP //,
			//PlaybackServer_Seek,
			//PlaybackServer_Restart, etc.
		};
		typedef PlaybackServerRequestCodes PlaybackServerRequestCode;

		///<summary>Used to service particular control messages.</summary>
		union PlaybackServerRequestData {
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
		Socket* socket;

		// Maintains the servers current state. Initial state is STOPPED.
		PlaybackServerState state;
		HANDLE serverMainThread, serverReceivingThread;
		HANDLE pausedStateSem;

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

		///<summary>Updates the client information table for the client that sent the check in.</summary>
		///<param name="data">The datagram data.</summary>
		///<param name="dataSize">The number of bytes in the datagram.</param>
		void receiveClientCheckIn(char* data, int dataSize);

		///<summary>This function will be called in order to notify the server that one or more 
		/// clients have moved, join, or dropped out, and that the volume must therefore be recalculated.</summary>
		void clientPositionsChanged();

		///<summary>Spawns a new thread to process audio and positional data. The thread will automatically
		/// terminate itself when finished.</summary>
		///<param name="audioFilename">The name of the audio file.</summary>
		///<param name="positionFilename">The name of the position information data file.</summary>
		void processAudioFilesOnThread(char* audioFilename, char* positionFilename);

		///<summary>Reads audio data from the file specified and returns it as an array.</summary>
		///<param name="filename">The name of the audio file.</param>
		///<param name="data">A pointer which will be filled with the address of the audio data array.</param>
		///<param name="size">A pointer which will be filled with the number of elements in the audio data array.</param>
		///<returns>TODO: Integer return code specifying the result of the call.</returns>
		int readAudioDataFromFile(char* filename, AudioSample** data, int* size);

		///<summary>Reads position data from the file specified and returns it as an array.</summary>
		///<param name="filename">The name of the position data file.</param>
		///<param name="data">A pointer which will be filled with the address of the position data array.</param>
		///<param name="size">A pointer which will be filled with the number of elements in the position data array.</param>
		///<returns>TODO: Integer return code specifying the result of the call.</returns>
		int readPositionDataFromFile(char* filename, PositionInfo** data, int* size);

		///<summary>Broadcasts an audio sample to the client network.</summary>
		///<param name="sampleBuffer">The sample data.</param>
		///<param name="sampleSize">The size of the sample data.</param>
		///<returns>TODO: Integer return code specifying the result of the call.</returns>
		int sendAudioSample(AudioSample sampleBuffer, int sampleSize);

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

	};


};