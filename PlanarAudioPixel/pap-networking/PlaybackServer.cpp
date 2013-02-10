#include "PlaybackServer.h"
#include "Socket.h"
#include "NetworkGlobals.h"
#include "ControlByteConstants.h"

namespace Networking {
		
		///<summary>Receives information from a client and stores it in the client information list.</summary>
		///<param name="data">The datagram data.</param>
		///<param name="dataSize">The number of bytes in the datagram.</param>
		void PlaybackServer::receiveClientConnection(char* data, int dataSize) {
			
			data[dataSize] = 0;

			//Acquire the client's broadcast IP and the client's locally unique ID.
			ClientGUID clientID;
			IP_Address BroadcastIP = *((IP_Address*)(data + 1));
			data += sizeof(IP_Address) + 1;
			IP_Address LocalIP = *((IP_Address*)(data + 1));
			data += sizeof(IP_Address);

			clientID.BroadcastIP = BroadcastIP;
			clientID.LocalIP = LocalIP;

			//Acquire the client's position;
			PositionInfo clientPosition;
			sscanf(data, "%f%f", &clientPosition.x, &clientPosition.y);

			//Build the client
			Client client;
			client.ClientID = clientID;
			client.Offset = clientPosition;
			client.LastCheckInTime = getMicroseconds();
			
			//Add the client to the information table
			Networking::ClientInformationTable[clientID] = client;

		}

		///<summary>Updates the client information table for the client that sent the check in.</summary>
		///<param name="data">The datagram data.</param>
		///<param name="dataSize">The number of bytes in the datagram.</param>
		void PlaybackServer::receiveClientCheckIn(char* data, int dataSize) {

			data[dataSize] = 0;

			//Acquire the client's broadcast IP and the client's locally unique ID.
			ClientGUID clientID;
			IP_Address BroadcastIP = *((IP_Address*)(data + 1));
			data += sizeof(IP_Address) + 1;
			IP_Address LocalIP = *((IP_Address*)(data + 1));
			data += sizeof(IP_Address);

			clientID.BroadcastIP = BroadcastIP;
			clientID.LocalIP = LocalIP;
			
			//Acquire the client's position;
			PositionInfo clientPosition;
			sscanf(data, "%f%f", &clientPosition.x, &clientPosition.y);

			//Update the timestamp
			Networking::ClientInformationTable[clientID].LastCheckInTime = getMicroseconds();


		}

		///<summary>Responds to a delay request sent by a client.</summary>
		///<param name="clientID">The ID of the client to send the response to.</param>
		void PlaybackServer::sendDelayResponseMessage(ClientGUID clientID) {

		}

		///<summary>Responds to an audio data resend request.</summary>
		///<param name="data">The datagram data.</param>
		///<param name="dataSize">The number of bytes in the datagram.</param>
		void PlaybackServer::resendAudio(char* data, int dataSize) {

		}

		///<summary>Responds to a volume data resend request.</summary>
		///<param name="data">The datagram data.</summary>
		///<param name="dataSize">The number of bytes in the datagram.</param>
		void PlaybackServer::resendVolume(char* data, int dataSize){

		}

		///<summary>This function will be called in order to notify the server that one or more clients have moved, join, or dropped out, and that the volume must therefore be recalculated.</summary>
		void PlaybackServer::clientPositionsChanged() {

		}
		
		///<summary>Adds the file names to a new track server request to be handled by serverMain().</summary>
		///<param name="audioFilename">The name of the audio file.</param>
		///<param name="positionFilename">The name of the position information data file.</param>
		void PlaybackServer::processAudioFiles(char* audioFilename, char* positionFilename) {
			PlaybackServerRequestData data;
			strcpy(data.newTrackInfo.audioFilename, audioFilename);
			strcpy(data.newTrackInfo.positionFilename, positionFilename);
			this->queueRequest(PlaybackServerRequestCodes::PlaybackServer_NEW_TRACK, data);
		}

		
		///<summary>Reads audio data from the file and fills an AudioBuffer.</summary>
		///<param name="filename">The name of the audio file.</param>
		///<param name="buffer">The buffer to fill.</param>
		///<returns>TODO: Integer return code specifying the result of the call.</returns>
		int PlaybackServer::readAudioDataFromFile(char* filename, AudioBuffer& buffer){
			if (!filename) return PlaybackServerErrorCodes::PlaybackServer_POINTER;

			//Attempt to open the file
			FILE* audioFile = fopen(filename, "rb");
			if (!audioFile) return PlaybackServerErrorCodes::PlaybackServer_FILE;

			//File reading buffer
			char readBuffer[4096];
			int readCount = 0;

			//Audio data struct setup
			IO::AudioData audioData;

			audioData.Data = (char*)malloc(sizeof(int) * 400 * 300);
			audioData.DataLength = 400 * 300;
			
			//Number of bytes a sample needs
			int bufferNeeds = 400 * 300 * sizeof(int);

			//Time counter
			time_t playbackOffset = 0;

			do {
				//Read 4096 bytes
				readCount = fread(readBuffer, 1, 4096, audioFile);
				if (!readCount) break;

				//Copy memory as appropriate
				if (bufferNeeds > readCount)
					memcpy(audioData.Data, readBuffer, 4096);
				else {
					memcpy(audioData.Data, readBuffer, bufferNeeds);

					AudioSample sample;
					sample.SampleID = buffer.size();
					sample.TimeOffset = playbackOffset;
					playbackOffset += 100000;
					sample.Data = audioData;

					buffer[buffer.size()] = sample;

					audioData.Data = (char*)malloc(sizeof(int) * 400 * 300);
					audioData.DataLength = 400 * 300;

					memcpy(audioData.Data, readBuffer, readCount - bufferNeeds);

					bufferNeeds = 400 * 300 * sizeof(int) - (readCount - bufferNeeds);
				}

			} while(readCount == 4096);

			//When the data fits perfectly, the last malloc is frivilous
			free(audioData.Data);

			return PlaybackServerErrorCodes::PlaybackServer_OK;
		}

		///<summary>Reads position data from the file and fills a PositionBuffer.</summary>
		///<param name="filename">The name of the position data file.</param>
		///<param name="buffer">The buffer to fill.</param>
		///<returns>TODO: Integer return code specifying the result of the call.</returns>
		int PlaybackServer::readPositionDataFromFile(char* filename, PositionBuffer& buffer){
			if (!filename) return PlaybackServerErrorCodes::PlaybackServer_POINTER;

			//Attempt to open the file
			FILE* audioFile = fopen(filename, "rb");
			if (!audioFile) return PlaybackServerErrorCodes::PlaybackServer_FILE;

			//File reading buffer
			char readBuffer[4096];
			int readCount = 0;

			//PositionInfo struct setup
			PositionInfo positionData;
			
			do {
				//Read 4096 bytes
				readCount = fread(readBuffer, 1, 4096, audioFile);
				if (!readCount) break;
				
				//Process position data
				for (int i = 0; i < readCount; i+=2*sizeof(float)){
					positionData.x = *((float*)(&readBuffer[i]));
					positionData.y = *((float*)(&readBuffer[i+sizeof(float)]));
					buffer[buffer.size()] = positionData;
				}

			} while(readCount == 4096);
			
			return PlaybackServerErrorCodes::PlaybackServer_OK;
		}

		///<summary>Broadcasts an audio sample to the client network.</summary>
		///<param name="sampleBuffer">The sample data.</param>
		///<param name="sampleSize">The size of the sample data.</param>
		///<returns>Integer return code specifying the result of the call.</returns>
		int PlaybackServer::sendAudioSample(AudioSample sampleBuffer, int sampleSize) {

			return E_FAIL;
		}

		///<summary>Broadcasts volume information for the client network.</summary>
		///<param name="segmentID">The ID of the segment to which this bit of volume data applies.</param>
		///<param name="trackID">The ID of the track to which this bit of volume data applies.</param>
		///<param name="volumeDataBuffer">The raw data containing the volume information.</param>
		///<param name="bufferSize">The number of bytes volume data.</param>
		///<returns>Integer return code specifying the result of the call.</returns>
		int PlaybackServer::sendVolumeData(sampleid_t segmentID, trackid_t trackID, char* volumeDataBuffer, int bufferSize) {

			return E_FAIL;
		}

		///<summary>Single entry point for all network communications. Reads the control byte and acts on it accordingly.</summary>
		///<param name="datagram">The datagram data.</param>
		///<param name="datagramSize">The size of the datagram.</param>
		void PlaybackServer::dispatchNetworkMessage(char* datagram, int datagramSize) {
			
			//switch on the control byte
			switch (datagram[0]){
			
				//A new connection.
			case Networking::ControlBytes::NEW_CONNECTION:
				this->receiveClientConnection(datagram, datagramSize);
				break;

				//A client check-in.
			case Networking::ControlBytes::PERIODIC_CHECK_IN:
				this->receiveClientCheckIn(datagram, datagramSize);
				break;

				//A client requesting an audio resend for a dropped packet.
			case Networking::ControlBytes::RESEND_AUDIO:
				this->resendAudio(datagram, datagramSize);
				break;

				//A client requesting a volume resend for a dropped packet.
			case Networking::ControlBytes::RESEND_VOLUME:
				this->resendAudio(datagram, datagramSize);
				break;

				//A pause-playback acknowledgement.
			case Networking::ControlBytes::PAUSE_PLAYBACK:

				break;

				//A resume-playback acknowledgement.
			case Networking::ControlBytes::RESUME_PLAYBACK:

				break;

				//A stop-playback acknowledgement.
			case Networking::ControlBytes::STOP_PLAYBACK:

				break;

			}

		}
		
		///<summary>Handles network communications and hands off incoming packets to dispatchNetworkMessage().</summary>
		void PlaybackServer::serverReceive(){

			//Receive data buffer
			char datagram[1500];

			while (this->state != PlaybackServerStates::PlaybackServer_STOPPED){

				//Try to receive data for 100ms
				int grams = this->socket->TryReceiveMessage(datagram, 1500, 100);

				//If grams == SocketError_TIMEOUT, the receive timed out. If grams == SOCKET_ERROR, there was a different error that can be retrieved with WSAGetLastError().
				if (grams > 0){
					//Hand off the packet
					this->dispatchNetworkMessage(datagram, grams);
				}

			}

		}
		///<summary>Multithreaded router function that calls serverReceieve().</summary>
		DWORD PlaybackServer::serverRouteReceive(void* server){
			static_cast<PlaybackServer*>(server)->serverReceive();
			return 0;
		}
		
		///<summary>Handles server management and control messages.</summary>
		void PlaybackServer::serverMain(){

			do {

				//Wait for a control message resource
				WaitForSingleObject(this->controlMessagesResourceCount, INFINITE);

				//Pop it off the queue
				EnterCriticalSection(&this->controlMessagesCriticalSection);
					PlaybackServerRequest request = this->controlMessages.front();
					this->controlMessages.pop();
				LeaveCriticalSection(&this->controlMessagesCriticalSection);

				switch (request.controlCode){

					//Load a new track
				case PlaybackServerRequestCodes::PlaybackServer_NEW_TRACK:
					TrackInfo newTrack;
					newTrack.TrackID = this->tracks.size() + 1;
					newTrack.currentPlaybackOffset = 0;
					
					//Read the audio file
					this->readAudioDataFromFile(request.controlData.newTrackInfo.audioFilename, newTrack.audioData);
					//Read the position file
					this->readPositionDataFromFile(request.controlData.newTrackInfo.positionFilename, newTrack.positionData);

					//Set the length of the track
					newTrack.trackLength = newTrack.audioData.size() * 100000;

					break;

				}

			} while (this->state != PlaybackServerStates::PlaybackServer_STOPPED);

		}
		///<summary>Multithreaded router function that calls serverMain().</summary>
		DWORD PlaybackServer::serverRouteMain(void* server){
			static_cast<PlaybackServer*>(server)->serverMain();
			return 0;
		}
		
		///<summary>Queues up a request code to be handled by serverMain().</summary>
		///<param name="code">The request code.</param>
		void PlaybackServer::queueRequest(PlaybackServerRequestCode code){

			//Create empty placeholder data
			PlaybackServerRequestData emptyData;
			memset(&emptyData, 0, sizeof(emptyData));

			queueRequest(code, emptyData);

		}
		
		///<summary>Queues up a request code to be handled by serverMain().</summary>
		///<param name="code">The request code.</param>
		///<param name="requestData">Additional data required to complete the request.</param>
		void PlaybackServer::queueRequest(PlaybackServerRequestCode code, PlaybackServerRequestData requestData){
			
			//Acquire the message queue
			EnterCriticalSection(&this->controlMessagesCriticalSection);
			
			//Construct the request
			PlaybackServerRequest newRequest;
			newRequest.controlCode = code;
			newRequest.controlData = requestData;
			this->controlMessages.push(newRequest);
			
			//Release the message queue
			LeaveCriticalSection(&this->controlMessagesCriticalSection);

			//Signal the new request
			ReleaseSemaphore(this->controlMessagesResourceCount, 1, NULL);

		}
		
		///<summary>Constructs a PlaybackServer object.</summary>
		PlaybackServer::PlaybackServer() {
			this->socket = NULL;
			this->serverMainThread = NULL;
			this->serverReceivingThread = NULL;
			this->state = PlaybackServerStates::PlaybackServer_STOPPED;
			InitializeCriticalSection(&this->controlMessagesCriticalSection);
			this->controlMessagesResourceCount = CreateSemaphore(NULL, 0, USHRT_MAX, NULL);
		}

		///<summary>Destructs a PlaybackServer object.</summary>
		PlaybackServer::~PlaybackServer(){
			if (this->socket){
				delete this->socket;
				this->socket = NULL;
			}
			if (this->serverMainThread)
				CloseHandle(this->serverMainThread);
			if (this->serverReceivingThread)
				CloseHandle(this->serverReceivingThread);
			CloseHandle(this->controlMessagesResourceCount);

			DeleteCriticalSection(&this->controlMessagesCriticalSection);
		}

		// ---------------------------------------------
		// PUBLIC METHODS
		// ---------------------------------------------
		
		///<summary>Attempts to start the PlaybackServer.</summary>
		///<returns>A PlaybackServer return code indicating the result of this call.</returns>
		int PlaybackServer::Start(){

			//god I hate the stl
			std::queue<PlaybackServerRequest> emptyMessageQueue;

			switch(this->state){
			case PlaybackServerStates::PlaybackServer_RUNNING:
				//If the server is already running, do nothing.
				return this->state;
				break;
			case PlaybackServerStates::PlaybackServer_STOPPED:

				//In case the server was running before and was stopped,
				//make sure that the threads had a chance to terminate.
				
			//================================================================================
				//The serverMain() SHOULD have stopped in order to arrive at this point.
				
				//If serverMain() has not stopped, attempt to signal it to terminate
				if (this->serverMainThread && WaitForSingleObject(this->serverMainThread, 1) == WAIT_TIMEOUT){
					this->queueRequest(PlaybackServerRequestCodes::PlaybackServer_STOP);

					//Wait for serverMain() to terminate. ---------------------------------------- We could throw an exception here after a certain amount of time?
					WaitForSingleObject(this->serverMainThread, INFINITE);
				}
			//================================================================================
			//================================================================================
				//The serverReceive() MAY still be in a TryReceiveMessage() call.
				WaitForSingleObject(this->serverReceivingThread, INFINITE);
			//================================================================================
				
				//Clear out the message queue
				EnterCriticalSection(&this->controlMessagesCriticalSection);
				std::swap(this->controlMessages, emptyMessageQueue);
				LeaveCriticalSection(&this->controlMessagesCriticalSection);

				//Set the state
				this->state = PlaybackServerStates::PlaybackServer_RUNNING;
				
				//Close previous thread handles
				if (this->serverMainThread)
					CloseHandle(this->serverMainThread);
				if (this->serverReceivingThread)
					CloseHandle(this->serverReceivingThread);

				//Create the worker threads
				this->serverMainThread = 
					CreateThread(
						NULL, 0, &PlaybackServer::serverRouteMain, static_cast<void*>(this), 0, NULL);
				this->serverReceivingThread = 
					CreateThread(
						NULL, 0, &PlaybackServer::serverRouteReceive, static_cast<void*>(this), 0, NULL);

				return PlaybackServerStates::PlaybackServer_RUNNING;

				break;
			}

			//The server is not in a valid state.
			return PlaybackServerErrorCodes::PlaybackServer_INVALID;
		}
		
		///<summary>Attempts to start playback.</summary>
		///<returns>A PlaybackErrorCode indicating the result of this call.</returns>
		int PlaybackServer::Play(){
			if (this->state != PlaybackServerStates::PlaybackServer_RUNNING) 
				return PlaybackServerErrorCodes::PlaybackServer_INVALID;

			this->queueRequest(PlaybackServerRequestCodes::PlaybackServer_PLAY);

			return PlaybackServerErrorCodes::PlaybackServer_OK;
		}

		///<summary>Attempts to create a PlaybackServer instance and returns an error code if it could not. fillServer is filled with NULL if creation fails.</summary>
		///<param name="fillServer">A reference to the PlaybackServer object to fill.</param>
		///<returns>A Networking::SocketErrorCode.</returns>
		int PlaybackServer::Create(PlaybackServer** fillServer){
			
			//Check the pointer, and initialize to NULL
			if (!fillServer) return Networking::SocketErrorCode::SocketError_POINTER;
			*fillServer = NULL;

			PlaybackServer* server = new PlaybackServer();

			//Attempt to create the socket inside the Server
			int socketCode = Socket::Create(&server->socket, Networking::SocketType::UDP, Networking::AddressFamily::IPv4);
			if (SOCKETFAILED(socketCode)){
				delete server;
				return socketCode;
			}

			//Attempt to set the socket up for UDP receiving
			socketCode = server->socket->PrepareUDPReceive(NetworkPort);
			if (SOCKETFAILED(socketCode)){
				delete server;
				return socketCode;
			}

			*fillServer = server;
			return Networking::SocketErrorCode::SocketError_OK;
		}
}