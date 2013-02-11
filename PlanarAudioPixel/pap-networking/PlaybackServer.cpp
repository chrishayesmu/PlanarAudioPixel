#include "PlaybackServer.h"
#include "Socket.h"
#include "NetworkGlobals.h"
#include "ControlByteConstants.h"

namespace Networking {
		
		///<summary>Receives information from a client and stores it in the client information list.</summary>
		///<param name="message">The message data.</param>
		///<param name="dataSize">The number of bytes in the datagram.</param>
		void PlaybackServer::receiveClientConnection(const PacketStructures::NetworkMessage* message, int dataSize) {
			
			//Build the client
			Client client;
			client.ClientID = message->ClientConnection.clientID;
			client.Offset = message->ClientConnection.position;
			client.LastCheckInTime = getMicroseconds();
			
			//Add the client to the information table
			Networking::ClientInformationTable[client.ClientID] = client;

		}

		///<summary>Updates the client information table for the client that sent the check in.</summary>
		///<param name="message">The message data.</param>
		///<param name="dataSize">The number of bytes in the datagram.</param>
		void PlaybackServer::receiveClientCheckIn(const PacketStructures::NetworkMessage* message, int dataSize) {
			
			//Update the timestamp
			Networking::ClientInformationTable[message->ClientConnection.clientID].LastCheckInTime = getMicroseconds();

			//TODO: Check if the position changed.

		}

		///<summary>Responds to a delay request sent by a client.</summary>
		///<param name="clientID">The ID of the client to send the response to.</param>
		void PlaybackServer::sendDelayResponseMessage(ClientGUID clientID) {

		}

		///<summary>Responds to an audio data resend request.</summary>
		///<param name="message">The message data.</param>
		///<param name="dataSize">The number of bytes in the datagram.</param>
		void PlaybackServer::resendAudio(const PacketStructures::NetworkMessage* message, int dataSize) {

		}

		///<summary>Responds to a volume data resend request.</summary>
		///<param name="message">The message data.</summary>
		///<param name="dataSize">The number of bytes in the datagram.</param>
		void PlaybackServer::resendVolume(const PacketStructures::NetworkMessage* message, int dataSize){

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
		///<param name="trackID">The ID of the track that this AudioSample belongs to.</param>
		///<param name="sampleBuffer">The sample data.</param>
		///<param name="bufferRangeStartID">The ID of the first sample in the buffer range currently being delivered.</param>
		///<param name="bufferRangeEndID">The ID of the last sample in the buffer range currently being delivered.</param>
		///<returns>TODO: Integer return code specifying the result of the call.</returns>
		int PlaybackServer::sendAudioSample(trackid_t trackID, AudioSample sampleBuffer, sampleid_t bufferRangeStartID, sampleid_t bufferRangeEndID){

			struct {
				PacketStructures::NetworkMessage networkHeader;
				char data[1500-32];
			} audioSampleMessage;

			audioSampleMessage.networkHeader.ControlByte = ControlBytes::SENDING_AUDIO;
			audioSampleMessage.networkHeader.AudioSample.SampleID = sampleBuffer.SampleID;
			audioSampleMessage.networkHeader.AudioSample.TrackID = trackID;
			audioSampleMessage.networkHeader.AudioSample.BufferRangeStartID = bufferRangeStartID;
			audioSampleMessage.networkHeader.AudioSample.BufferRangeEndID = bufferRangeEndID;

			return E_FAIL;
		}

		///<summary>Broadcasts volume information for the client network.</summary>
		///<param name="trackID">The ID of the track to which this bit of volume data applies.</param>
		///<param name="sampleID">The ID of the sample to which this bit of volume data applies.</param>
		///<param name="bufferRangeStartID">The ID of the first sample in the buffering range.</param>
		///<param name="bufferRangeEndID">The ID of the last sample in the buffering range.</param>
		///<returns>Integer return code specifying the result of the call.</returns>
		int PlaybackServer::sendVolumeData(trackid_t trackID, sampleid_t sampleID, VolumeInfo volumeData, sampleid_t bufferRangeStartID, sampleid_t bufferRangeEndID) {

			return E_FAIL;
		}

		///<summary>Single entry point for all network communications. Reads the control byte and acts on it accordingly.</summary>
		///<param name="message">The network message.</param>
		///<param name="datagramSize">The size of the datagram.</param>
		void PlaybackServer::dispatchNetworkMessage(const PacketStructures::NetworkMessage* message, int datagramSize) {
			
			//switch on the control byte
			switch (message->ControlByte){
			
				//A new connection.
			case Networking::ControlBytes::NEW_CONNECTION:
				this->receiveClientConnection(message, datagramSize);
				break;

				//A client check-in.
			case Networking::ControlBytes::PERIODIC_CHECK_IN:
				this->receiveClientCheckIn(message, datagramSize);
				break;

				//A client requesting an audio resend for a dropped packet.
			case Networking::ControlBytes::RESEND_AUDIO:
				if (!this->initialBuffering)
					this->resendAudio(message, datagramSize);
				else { 
					//If we're in the initial buffering phase, check off that a sample didn't make it: "needs to be resent"
					this->samplesResend[message->AudioResendRequest.SampleID] = true;
				}
				break;

				//A client requesting a volume resend for a dropped packet.
			case Networking::ControlBytes::RESEND_VOLUME:
				if (!this->initialBuffering)
					this->resendVolume(message, datagramSize);
				else { 
					//If we're in the initial buffering phase, check off that a sample didn't make it: "needs to be resent"
					this->samplesResend[message->VolumeResendRequest.SampleID] = true;
				}
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
			struct {
				PacketStructures::NetworkMessage message;
				char data[1500-32];
			} NetworkPacket;

			while (this->state != PlaybackServerStates::PlaybackServer_STOPPED){

				//Try to receive data for 100ms
				int grams = this->recvSocket->TryReceiveMessage((char*)&NetworkPacket, 1500, 100);

				//If grams == SocketError_TIMEOUT, the receive timed out. If grams == SOCKET_ERROR, there was a different error that can be retrieved with WSAGetLastError().
				if (grams > 0){
					//Hand off the packet
					this->dispatchNetworkMessage(&NetworkPacket.message, grams);
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

				//Play-Request variables
				std::map<ClientGUID, Client> clients;
				void (*bufferingCallback)(float percentBuffered);
				int bufferMax = 0;
				int bufferCount = 0;
				//==============

				switch (request.controlCode){

					//Play all tracks. This code path is only taken if the playback state is STOPPED. Otherwise, RESUME is taken.
				case PlaybackServerRequestCodes::PlaybackServer_PLAY:

					//Indicate that this is the initial buffering phase
					this->initialBuffering = true;

					//Acquire a local copy of the list of clients
					clients = ClientInformationTable;

					bufferingCallback = request.controlData.bufferingCallback;
					
					//Count the number of samples to buffer
					for (unsigned int i = 0; i < tracks.size(); ++i) {
						bufferMax += min(tracks[i].audioSamples.size(), RequiredBufferedSamplesCount) * 2;
					}

					//Send the first RequiredBufferedSamplesCount samples and accompanying volume information
					//and ensure that they arrive.
					for (unsigned int i = 0; i < tracks.size(); ++i) 
					{

						//If there are less samples in the track than the required initial buffering amount, buffer the entire
						//track.
						unsigned int trackBufferSize = min(tracks[i].audioSamples.size(), RequiredBufferedSamplesCount);

						//Add each sample as "not needing to be resent"
						this->samplesResend.clear();
						this->volumesResend.clear();
						for (unsigned int j = 0; j < trackBufferSize; ++j){
							this->samplesResend[j] = false;
							this->volumesResend[j] = false;
						}

						//Send samples
						do {
							//Iterate over the keys left in sampleReceivedCounts (those are the sampleid_t's of the samples that haven't successfully buffered),
							//and attempt to buffer them.
							for (SampleID_Bool_I j = this->samplesResend.begin(); j != this->samplesResend.end(); ++j){
								this->sendAudioSample(i, tracks[i].audioSamples[j->first], 0, trackBufferSize);
							}
														
							//Wait for the amount of time that should indicate that every client either recieved or did not recieve one of the packets
							Networking::busyWait(Networking::ClientReceivedPacketTimeout);

							std::map<sampleid_t, bool> tempSampleResend;

							//Add samples back that didn't make it
							for (SampleID_Bool_I j = this->samplesResend.begin(); j != this->samplesResend.end(); ++j){
								if (j->second)
									tempSampleResend[j->first] = false;
							}
							this->samplesResend = tempSampleResend;

						} while (this->samplesResend.size());

						//Send volume data
						do {
							//Iterate over the keys left in sampleReceivedCounts (those are the sampleid_t's of the samples that haven't successfully buffered),
							//and attempt to buffer them.
							for (SampleID_Bool_I j = this->volumesResend.begin(); j != this->volumesResend.end(); ++j){
								this->sendVolumeData(i, j->first, tracks[i].volumeData[j->first], 0, trackBufferSize);
							}
							
							//Wait for the amount of time that should indicate that every client either recieved or did not recieve one of the packets
							Networking::busyWait(Networking::ClientReceivedPacketTimeout);

							std::map<sampleid_t, bool> tempVolumeResend;

							//Add samples back that didn't make it
							for (SampleID_Bool_I j = this->volumesResend.begin(); j != this->volumesResend.end(); ++j){
								if (j->second)
									tempVolumeResend[j->first] = false;
							}
							this->samplesResend = tempVolumeResend;

						} while (this->samplesResend.size());

					}

					//Buffering is complete
					this->initialBuffering = false;

					//Send PLAY control packets

					break;

					//Load a new track
				case PlaybackServerRequestCodes::PlaybackServer_NEW_TRACK:
					TrackInfo newTrack;
					newTrack.TrackID = this->tracks.size() + 1;
					newTrack.currentPlaybackOffset = 0;
					newTrack.samplesBuffered = 0;
					
					//Read the audio file
					this->readAudioDataFromFile(request.controlData.newTrackInfo.audioFilename, newTrack.audioSamples);
					//Read the position file
					this->readPositionDataFromFile(request.controlData.newTrackInfo.positionFilename, newTrack.positionData);

					//Set the length of the track
					newTrack.trackLength = newTrack.audioSamples.size() * 100000;

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
			this->recvSocket = NULL;
			this->sendSocket = NULL;
			this->serverMainThread = NULL;
			this->serverReceivingThread = NULL;
			this->state = PlaybackServerStates::PlaybackServer_STOPPED;
			this->initialBuffering = false;
			InitializeCriticalSection(&this->controlMessagesCriticalSection);
			this->controlMessagesResourceCount = CreateSemaphore(NULL, 0, USHRT_MAX, NULL);
		}

		///<summary>Destructs a PlaybackServer object.</summary>
		PlaybackServer::~PlaybackServer(){
			if (this->recvSocket){
				delete this->recvSocket;
				this->recvSocket = NULL;
			}
			if (this->sendSocket){
				delete this->sendSocket;
				this->sendSocket = NULL;
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

			//Attempt to create the sendSocket inside the Server
			int sendSocketCode = Socket::Create(&server->sendSocket, Networking::SocketType::UDP, Networking::AddressFamily::IPv4);
			if (SOCKETFAILED(sendSocketCode)){
				delete server;
				return sendSocketCode;
			}

			//Set the sendSocket up for UDP broadcasting
			server->sendSocket->PrepareUDPSend(NetworkPort, "127.0.0.1");

			//Attempt to create the recvSocket inside the Server
			int recvSocketCode = Socket::Create(&server->recvSocket, Networking::SocketType::UDP, Networking::AddressFamily::IPv4);
			if (SOCKETFAILED(recvSocketCode)){
				delete server;
				return recvSocketCode;
			}

			//Attempt to set the recvSocket up for UDP receiving
			recvSocketCode = server->recvSocket->PrepareUDPReceive(NetworkPort);
			if (SOCKETFAILED(recvSocketCode)){
				delete server;
				return recvSocketCode;
			}

			*fillServer = server;
			return Networking::SocketErrorCode::SocketError_OK;
		}
}