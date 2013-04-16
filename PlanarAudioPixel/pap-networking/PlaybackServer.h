#include "NetworkStructures.h"
#include "Socket.h"
#include "sockets.h"
#include <queue>

#define close _close
//Giancarlo

namespace Networking {

	///<summary>A set of PlaybackServer states.</summary>
	enum PlaybackServerStates {
		PlaybackServer_RUNNING = 1,
		PlaybackServer_INVALID = 2,
		PlaybackServer_STOPPED = 3
	};
	typedef PlaybackServerStates PlaybackServerState;

	///<summary>A set of playback states.</summary>
	enum PlaybackStates {
		Playback_INVALID = -1,
		Playback_PLAYING = 1,
		Playback_PAUSED = 2,
		Playback_STOPPED = 3
	};
	typedef PlaybackStates PlaybackState;

	///<summary>A set of PlaybackServer error codes for the return values of certain functions.</summary>
	enum PlaybackServerErrorCodes {
		PlaybackServer_OK = 0,
		PlaybackServer_ISINVALID = 1,
		PlaybackServer_FILE = 2,
		PlaybackServer_POINTER = 3
	};
	typedef PlaybackServerErrorCodes PlaybackServerErrorCode;

	/// Public typedefs for callback functions
	typedef void (*AddTrackCallback)(int audioCode, int positionCode, uint64_t token);
	typedef void (*ClientConnectedCallback)(Client c);
	typedef void (*ClientDisconnectedCallback)(ClientGUID c);

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
			PlaybackServer_NEW_TRACK,
			PlaybackServer_TIMER_TICK
		};
		typedef PlaybackServerRequestCodes PlaybackServerRequestCode;

		///<summary>Used to service particular control messages.</summary>
		union PlaybackServerRequestData {

			//Information used to queue a NEW_TRACK request
			struct {
				char audioFilename[256];
				char positionFilename[256];

				//Callback information
				uint64_t token;
				AddTrackCallback callback;
			} newTrackInfo;

			//Information used to queue a BUFFER request
			struct {
				trackid_t trackID;
				sampleid_t beginBufferRange;
				sampleid_t endBufferRange;
			} bufferInfo;

			//Information used when buffering during the initial PLAY request
			struct {
				void (*bufferingCallback)(float percentBuffered);
			} initialBufferingInfo;
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

		// Main server socket
		_socket* serverSocket;

		// Maintains the servers current state. Initial state is STOPPED.
		PlaybackServerState state;
		HANDLE serverMainThread, serverReceivingThread;

		PlaybackState playbackState;
		time_t playbackOffset;
				
		// Used to indicate whether or not a timer event needs to be consumed.
		// The timer tick event will not produce a new tick until timerTicked is reset to false.
		bool timerTicked;
		
		///<summary>Produces server timer-tick events.</summary>
		void timerTickEvent();
		///<summary>Multithreaded router function that calls timerTickEvent().</summary>
		static DWORD __stdcall timerRoute(void* server);

		// Used to give the timer a chance to complete when the server object destructs.
		CRITICAL_SECTION timerDestroyedCriticalSection;

		// Track buffer object
		TrackBuffer tracks;

		// Clients
		typedef std::map<ClientGUID, Client>::iterator ClientIterator;
		std::map<ClientGUID, Client> clients;

		// Callback subscriptions
		std::vector<ClientConnectedCallback> clientConnectedCallbacks;
		std::vector<ClientDisconnectedCallback> clientDisconnectedCallbacks;

		//Construction/destruction
		PlaybackServer();
		~PlaybackServer();

		///<summary>Reads audio data from the file and fills an AudioBuffer.</summary>
		///<param name="filename">The name of the audio file.</param>
		///<param name="buffer">The buffer to fill.</param>
		///<returns>Size of the file that was read in.</returns>
		int readAudioDataFromFile(char* filename, AudioBuffer& buffer);

		///<summary>Reads position data from the file and fills a PositionBuffer.</summary>
		///<param name="filename">The name of the position data file.</param>
		///<param name="buffer">The buffer to fill.</param>
		///<returns>TODO: Integer return code specifying the result of the call.</returns>
		int readPositionDataFromFile(char* filename, PositionBuffer& buffer);

		///<summary>Calculates the volume for a track, for all sample IDs between sampleStart and
		/// sampleEnd, inclusive. The volume is stored in this->tracks.</summary>
		///<param name='sampleStart'>The sample ID to begin calculating volume at.</param>
		///<param name='sampleEnd'>The sample ID to be the last volume calculation.</param>
		void calculateVolumeData(trackid_t trackID, sampleid_t sampleStart, sampleid_t sampleEnd);

		//## Possibly add an RTT field to the Client structure and periodically send RTT tracking requests. Then use it 
		//## to send an accurate expected wait time to a Client for receiving all samples in a buffer range.
		///<summary>Broadcasts an audio sample to the client network.</summary>
		///<param name="trackID">The ID of the track that this AudioSample belongs to.</param>
		///<param name="sampleBuffer">The sample data.</param>
		///<param name="bufferIDStart">The ID of the first sample in the buffer range currently being delivered.</param>
		///<param name="bufferIDEnd">The ID of the last sample in the buffer range currently being delivered.</param>
		void sendAudioSample(trackid_t trackID, AudioSample sampleBuffer, sampleid_t bufferIDStart, sampleid_t bufferIDEnd);

		///<summary>Broadcasts volume information for the client network.</summary>
		///<param name="trackID">The ID of the track to which this bit of volume data applies.</param>
		///<param name="sampleID">The ID of the sample to which this bit of volume data applies.</param>
		///<param name="bufferRangeStartID">The ID of the first sample in the buffering range.</param>
		///<param name="bufferRangeEndID">The ID of the last sample in the buffering range.</param>
		void sendVolumeData(trackid_t trackID, sampleid_t sampleID, VolumeInfo volumeData, sampleid_t bufferRangeStartID, sampleid_t bufferRangeEndID);

		void broadcastMessage(const void* __restrict data, int size);

		///<summary>Sends a disconnect packet to a particular client.</summary>
		///<param name="index">The ID of the client to drop.</param>
		void sendDisconnect(ClientGUID ID);
		
		///<summary>Handles network communications.</summary>
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
		///<returns>A PlaybackServerErrorCode indicating the result of this call.</returns>
		PlaybackServerErrorCode ServerStart();

		///<summary>Attempts to start playback.</summary>
		///<returns>A PlaybackErrorCode indicating the result of this call.</returns>
		PlaybackServerErrorCode Play();

		///<summary>Attempts to pause playback.</summary>
		///<returns>A PlaybackErrorCode indicating the result of this call.</returns>
		PlaybackServerErrorCode Pause();

		///<summary>Attempts to stop playback.</summary>
		///<returns>A PlaybackErrorCode indicating the result of this call.</returns>
		PlaybackServerErrorCode Stop();
		
		///<summary>Queues up a request to asynchronously add a track to the server's playlist.</summary>
		///<param name="audioFilename">The name of the audio file to add.</param>
		///<param name="positionFilename">The name of the corresponding position data file.</param>
		///<returns>A PlaybackServerErrorCode indicating the result of this call.</returns>
		PlaybackServerErrorCode AddTrack(char* audioFilename, char* positionFilename);
		
		///<summary>Queues up a request to asynchronously add a track to the server's playlist.</summary>
		///<param name="audioFilename">The name of the audio file to add.</param>
		///<param name="positionFilename">The name of the corresponding position data file.</param>
		///<param name="callback">The callback function to call when the corresponding request completes.</param>
		///<param name="token">An identifying token that is passed along with the callback when the corresponding request complete.</param>
		///<returns>A PlaybackServerErrorCode indicating the result of this call.</returns>
		PlaybackServerErrorCode AddTrack(char* audioFilename, char* positionFilename, AddTrackCallback callback, uint64_t token);
		
		///<summary>Subscribes the caller to the ClientConnected event. ClientConnected is raised when a new client appears on the network.</summary>
		///<param name="callback">A pointer to the function to call when the event is raised.</param>
		void OnClientConnected(ClientConnectedCallback callback);

		///<summary>Subscribes the caller to the ClientDisconnected event. ClientDisconnected is raised when a client disconnects from the network.</summary>
		///<param name="callback">A pointer to the function to call when the event is raised.</param>
		void OnClientDisconnected(ClientDisconnectedCallback callback);

	};
};