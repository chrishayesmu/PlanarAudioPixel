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

			//Information used to queue a NEW_TRACK request
			struct {
				char audioFilename[256];
				char positionFilename[256];

				//Callback information
				uint64_t token;
				void (*callback)(int audioErrorCode, int positionErrorCode, uint64_t token);
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

		// Main receiving and sending socket
		Socket* recvSocket, * sendSocket;

		// Maintains the servers current state. Initial state is STOPPED.
		PlaybackServerState state;
		HANDLE serverMainThread, serverReceivingThread;

		PlaybackState playbackState;

		// Track buffer object
		TrackBuffer tracks;

		// Used to ensure delivery of the initial buffering samples
		typedef std::map<sampleid_t,  bool>::iterator BufferingIterator;
		std::map<sampleid_t, bool> samplesResend;
		std::map<sampleid_t, bool> volumesResend;
		bool initialBuffering;

		// Used to ensure transport controls are processed
		requestid_t currentRequestID;
		typedef std::map<requestid_t, std::map<ClientGUID, bool>>::iterator AcknowledgementIterator;
		typedef std::map<ClientGUID, bool>::iterator ClientAcknowledgementIterator;

		//requestid_t -> (ClientGUID -> bool)
		std::map<requestid_t, std::map<ClientGUID, bool>> requestsAcknowledged;

		//Construction/destruction
		PlaybackServer();
		~PlaybackServer();

		///<summary>Receives information from a client and stores it in the client information list.</summary>
		///<param name="data">The message data.</param>
		///<param name="dataSize">The number of bytes in the datagram.</param>
		void receiveClientConnection(const PacketStructures::NetworkMessage* message, int dataSize);

		///<summary>Responds to a delay request sent by a client.</summary>
		///<param name="clientID">The ID of the client to send the response to.</param>
		void sendDelayResponseMessage(ClientGUID clientID);

		///<summary>Responds to an audio data resend request.</summary>
		///<param name="data">The message data.</summary>
		///<param name="dataSize">The number of bytes in the datagram.</param>
		void resendAudio(const PacketStructures::NetworkMessage* message, int dataSize);

		///<summary>Responds to a volume data resend request.</summary>
		///<param name="data">The message data.</summary>
		///<param name="dataSize">The number of bytes in the datagram.</param>
		void resendVolume(const PacketStructures::NetworkMessage* message, int dataSize);

		///<summary>Updates the client information table for the client that sent the check in.</summary>
		///<param name="data">The message data.</summary>
		///<param name="dataSize">The number of bytes in the datagram.</param>
		void receiveClientCheckIn(const PacketStructures::NetworkMessage* message, int dataSize);

		///<summary>This function will be called in order to notify the server that one or more 
		/// clients have moved, join, or dropped out, and that the volume must therefore be recalculated.</summary>
		void clientPositionsChanged();

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
		
		///<summary>Single entry point for all network communications. Reads the control byte and acts on it accordingly.</summary>
		///<param name="datagram">The network message.</param>
		///<param name="datagramSize">The size of the datagram.</param>
		void dispatchNetworkMessage(const PacketStructures::NetworkMessage* message, int datagramSize);
		
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
		///<returns>A PlaybackServerErrorCode indicating the result of this call.</returns>
		PlaybackServerErrorCode Start();

		///<summary>Attempts to start playback.</summary>
		///<returns>A PlaybackErrorCode indicating the result of this call.</returns>
		PlaybackServerErrorCode Play();
		
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
		PlaybackServerErrorCode AddTrack(char* audioFilename, char* positionFilename, void (*callback)(int audioCode, int positionCode, uint64_t token), uint64_t token);

	};


};