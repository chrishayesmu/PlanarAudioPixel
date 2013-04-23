#include "PlaybackServer.h"
#include "Socket.h"
#include "NetworkGlobals.h"
#include "ControlByteConstants.h"
#include "NetworkConstants.h"
#include "../pap-file-io/Logger.h"

#include <time.h>

#pragma comment(lib, "../Debug/pap-file-io.lib")

namespace Networking {
		
				
		///<summary>Reads audio data from the file and fills an AudioBuffer.</summary>
		///<param name="filename">The name of the audio file.</param>
		///<param name="buffer">The buffer to fill.</param>
		///<returns>TODO: Integer return code specifying the result of the call.</returns>
		int PlaybackServer::readAudioDataFromFile(char* filename, AudioBuffer& buffer){
			if (!filename) return PlaybackServerErrorCodes::PlaybackServer_POINTER;

			Logger::logNotice("Attempting to read audio data from file %s ..", filename);

			//Attempt to open the file
			FILE* audioFile = fopen(filename, "rb");
			if (!audioFile) 
			{
				Logger::logWarning("Failed to open audio file %s", filename);
				return -1;
			}

			char* dataBuffer[Networking::SampleSize];
			uint32_t readCount = 0;
			int ID = 0;
			uint32_t readTotal = 0;
			do {
				readCount = fread((void*)dataBuffer, 1, Networking::SampleSize, audioFile);
				memcpy(buffer[ID].Data.Data, dataBuffer, readCount);
				buffer[ID].Data.DataLength = readCount;
				buffer[ID].SampleID = ID;
				++ID;
				readTotal += readCount;
			} while (readCount == (Networking::SampleSize));

			Logger::logNotice("Successfully read audio file %s", filename);
			return (int)readTotal;
		}

		///<summary>Reads position data from the file and fills a PositionBuffer.</summary>
		///<param name="filename">The name of the position data file.</param>
		///<param name="buffer">The buffer to fill.</param>
		///<returns>TODO: Integer return code specifying the result of the call.</returns>
		PlaybackServerErrorCode PlaybackServer::readPositionDataFromFile(const char* filename, int sampleCount, PositionBuffer& buffer){
			if (!filename) return PlaybackServerErrorCodes::PlaybackServer_POINTER;

			Logger::logNotice("Attempting to read volume data from file %s ..", filename);

			//Attempt to open the file
			FILE* audioFile = fopen(filename, "rb");
			if (!audioFile) 
			{
				Logger::logWarning("Failed to open volume file %s", filename);
				return PlaybackServerErrorCodes::PlaybackServer_FILE;
			}

			//Parse the window resolution as the first pair of values
			float width, height;
			fscanf(audioFile, "%f %f", &width, &height);
			
			//Continue parsing each pair of position data
			std::vector<Networking::PositionInfo> data;
			float x, y;
			while (fscanf(audioFile, "%f %f", &x, &y)) {
				PositionInfo info = {x / width, y / height};
				data.push_back(info);
				char c;
				if ((c=getc(audioFile)) == EOF) break;
			}

			//Stretch out the number of position samples linearly to match the number of audio samples
			//Assuming there are more audio samples than position samples
			float ratio = (float)data.size() / sampleCount;
			float bufferPos = 0;
			for (int i = 0; i < sampleCount; ++i) {
				buffer[i] = data[(int)bufferPos];
				bufferPos += ratio;
			}
			
			Logger::logNotice("Successfully read volume file %s", filename);
			return PlaybackServerErrorCodes::PlaybackServer_OK;
		}

		///<summary>Calculates the volume for a track, for all sample IDs between sampleStart and
		/// sampleEnd, inclusive. The volume is stored in this->tracks.</summary>
		void PlaybackServer::calculateVolumeData(trackid_t trackID)
		{
			// Local variables because I'm lazy
			TrackInfo track = this->tracks[trackID];
			PositionBuffer positions = track.positionData;

			// Naive algorithm: scale everything so that the nearest client plays at full volume,
			// then scale down from there. No regard given to units or any anomalous situations that may arise.
			// This algorithm will be modified and refined as testing shows how well it works.
			sampleid_t sampleStart =  0, sampleEnd = this->tracks[trackID].audioSamples.size();
			for (sampleid_t i = sampleStart; i < sampleEnd; i++)
			{
				PositionInfo pos = positions[i];
				ClientGUID closestClient = this->clients.begin()->first;
				ClientGUID furthestClient = closestClient;

				// Map between ClientGUIDs and their distance from the audio source
				std::map<ClientGUID, float> clientDistMap;

				// Record each client's distance from the source
				for (ClientIterator clientIt = this->clients.begin(); clientIt != this->clients.end(); clientIt++)
				{
					Client client = clientIt->second;
					float dist = sqrt((pos.x - client.Offset.x) * (pos.x - client.Offset.x) + (pos.y - client.Offset.y) * (pos.y - client.Offset.y));

					clientDistMap[clientIt->first] = dist;

					if (dist < clientDistMap[closestClient])
					{
						closestClient = clientIt->first;
					}

					if (dist > clientDistMap[furthestClient])
					{
						furthestClient = clientIt->first;
					}
				}

				// Min and max found; record those now before the next loop can modify them
				float minDist = clientDistMap[closestClient];
				float maxDist = clientDistMap[furthestClient];

				// Now go back through and scale each distance to fit in the range [0, 1];
				// apply clipping to any volumes falling below a threshold
				for (ClientIterator clientIt = this->clients.begin(); clientIt != this->clients.end(); clientIt++)
				{
					clientDistMap[clientIt->first] = 1 ;// (clientDistMap[clientIt->first] - minDist) / (maxDist - minDist);

					if (clientDistMap[clientIt->first] < MIN_VOLUME_THRESHOLD)
						clientDistMap[clientIt->first] = 0.0f;

					// Store volume data in global data
					track.volumeData[sampleStart][clientIt->first] = clientDistMap[clientIt->first];
				}
			}
		}

		void PlaybackServer::broadcastMessage(const void* __restrict data, int size) {
			//Broadcast the message
			for (ClientIterator i = this->clients.begin(); i != this->clients.end(); ++i) {
				int sendSize = 0;
				if ((sendSize = sockets_send_message(
						i->second.s,
						data,
						size)) == -1) {
					//If the send size was 0, assume the socket connection was dropped
					for (unsigned int j = 0; j < this->clientConnectedCallbacks.size(); ++j) {
						this->clientDisconnectedCallbacks[j](i->first);
					}

				}
			}									
		}

		///<summary>Broadcasts an audio sample to the client network.</summary>
		///<param name="trackID">The ID of the track that this AudioSample belongs to.</param>
		///<param name="sampleBuffer">The sample data.</param>
		///<param name="bufferRangeStartID">The ID of the first sample in the buffer range currently being delivered.</param>
		///<param name="bufferRangeEndID">The ID of the last sample in the buffer range currently being delivered.</param>
		///<returns>TODO: Integer return code specifying the result of the call.</returns>
		void PlaybackServer::sendAudioSample(trackid_t trackID, AudioSample sampleBuffer, sampleid_t bufferRangeStartID, sampleid_t bufferRangeEndID){
		
			// TODO - log buffer start/end? Not clear on what these things are
			Logger::logNotice("Sending audio sample; trackID: %d; sampleID: %d", trackID, sampleBuffer.SampleID);

			//Define a struct capable of containing both the network message and the buffer data.
			struct {
				PacketStructures::NetworkMessage networkHeader;
				char data[Networking::SampleSize];
			} audioSampleMessage;

			audioSampleMessage.networkHeader.ControlByte = ControlBytes::SENDING_AUDIO;
			audioSampleMessage.networkHeader.AudioSample.SampleID = sampleBuffer.SampleID;
			audioSampleMessage.networkHeader.AudioSample.TrackID = trackID;
			audioSampleMessage.networkHeader.AudioSample.fileSize = this->tracks[trackID].fileSize;

			//Assume that the function constructing the sample buffer behaved well and didn't produce a sample with greater than 1468 bytes.
			memcpy(audioSampleMessage.data, sampleBuffer.Data.Data, sampleBuffer.Data.DataLength);

			//Include the length of the data in this message.
			audioSampleMessage.networkHeader.Extra._dataLength = sampleBuffer.Data.DataLength;

			//Broadcast the data
			this->broadcastMessage(&audioSampleMessage, sizeof(PacketStructures::NetworkMessage) + sampleBuffer.Data.DataLength);
		}

		///<summary>Broadcasts volume information for the client network.</summary>
		///<param name="trackID">The ID of the track to which this bit of volume data applies.</param>
		///<param name="sampleID">The ID of the sample to which this bit of volume data applies.</param>
		///<returns>Integer return code specifying the result of the call.</returns>
		void PlaybackServer::sendVolumeData(trackid_t trackID, sampleid_t sampleID) {
						
					/// ----- THIS NEEDS TO BE BROADCAST ------
			// TODO - log buffer start/end? Not clear on what these things are
			Logger::logNotice("Sending volume sample; trackID: %d; sampleID: %d", trackID, sampleID);

			//Define a struct capable of containing both the network message and 121 clients of volume data.
			// TODO what happens if there's more than 121 clients worth of data? We need to send multiple messages
			struct {
				PacketStructures::NetworkMessage networkHeader;
				PacketStructures::ClientVolume volumeData[121];
			} volumeDataMessage;

			volumeDataMessage.networkHeader.ControlByte = ControlBytes::SENDING_VOLUME;
			volumeDataMessage.networkHeader.VolumeSample.SampleID = sampleID;
			volumeDataMessage.networkHeader.VolumeSample.TrackID = trackID;

			for (ClientIterator i = this->clients.begin(); i != this->clients.end(); ++i) {
				int sendSize = 0;

				volumeDataMessage.networkHeader.VolumeSample.volume = tracks[trackID].volumeData[sampleID][i->first];

				if ((sendSize = sockets_send_message(
						i->second.s,
						&volumeDataMessage.networkHeader,
						sizeof(volumeDataMessage.networkHeader))) == -1) {
					//If the send size was 0, assume the socket connection was dropped
					for (unsigned int j = 0; j < this->clientConnectedCallbacks.size(); ++j) {
						this->clientDisconnectedCallbacks[j](i->first);
					}

				}
			}

		}

		///<summary>Sends a disconnect packet to a particular client.</summary>
		///<param name="index">The index in the client list of the client to drop.</param>
		void PlaybackServer::sendDisconnect(ClientGUID ID) {
			
			Logger::logNotice("Sending disconnect message to client (ID: %d)", ID);

			PacketStructures::NetworkMessage disconnectMessage;
			disconnectMessage.ControlByte = ControlBytes::DISCONNECT;
			
			if (this->clients.find(ID) != this->clients.end()) {
				sockets_send_message(this->clients[ID].s, &disconnectMessage, sizeof(PacketStructures::NetworkMessage));
				closesocket(this->clients[ID].s);
				this->clients.erase(this->clients.find(ID));

				//Send the disconnect message
				for (unsigned int j = 0; j < this->clientConnectedCallbacks.size(); ++j) {
					this->clientDisconnectedCallbacks[j](ID);
				}

			}

		}
		
		///<summary>Handles network communications and hands off incoming packets to dispatchNetworkMessage().</summary>
		void PlaybackServer::serverReceive(){

			//CONNECT CLIENTS
			while (this->state != PlaybackServerStates::PlaybackServer_STOPPED) {
				
				//Attempt to connect new clients for up to 1 second. Timeouts give the server a chance to evaluate state.
				int acceptClient;
				if ((acceptClient = sockets_accept(this->serverSocket, 1000)) != -1) {
					
					PacketStructures::NetworkMessage connectMessage;
					sockets_receive_message(acceptClient, &connectMessage, sizeof(connectMessage));

					Client c;
					c.ClientID = connectMessage.ClientConnection.clientID;
					c.Offset = connectMessage.ClientConnection.position;
					c.s = acceptClient;

					this->clients[c.ClientID] = c;

					//Send the client connected
					for (unsigned int j = 0; j < this->clientConnectedCallbacks.size(); ++j) {
						this->clientConnectedCallbacks[j](c);
					}

				}

			}

		}
		///<summary>Multithreaded router function that calls serverReceive().</summary>
		DWORD PlaybackServer::serverRouteReceive(void* server){
			static_cast<PlaybackServer*>(server)->serverReceive();
			return 0;
		}
		
		///<summary>Handles server management and control messages.</summary>
		void PlaybackServer::serverMain(){
			this->playbackState = PlaybackStates::Playback_STOPPED;

			Logger::logNotice("Starting playback server");

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
				int bufferCount = 0;
				PacketStructures::NetworkMessage playMessage;
				bool resend = true;
				//==============
				
				//Buffer-Request variables
				sampleid_t beginBufferRange = request.controlData.bufferInfo.beginBufferRange;
				sampleid_t endBufferRange = request.controlData.bufferInfo.endBufferRange;
				trackid_t  bufferTrackID = request.controlData.bufferInfo.trackID;
				//==============
				
				//Transport-Request variables
				PacketStructures::NetworkMessage transportControlRequest;
				//==============

				switch (request.controlCode){

				//Process timer tick events
				case PlaybackServerRequestCodes::PlaybackServer_TIMER_TICK:


					//Indicate that this timer tick was consumed
					this->timerTicked = false;

					break;

					//Continually buffer a particular track
				case PlaybackServerRequestCodes::PlaybackServer_BUFFER:
					
					//If we're stopped, ignore BUFFER requests
					if (this->playbackState == PlaybackStates::Playback_STOPPED) break;

					//Buffer the next range of samples
					for (unsigned int i = beginBufferRange; i < endBufferRange; ++i) {
						this->sendAudioSample(bufferTrackID, 
												this->tracks[bufferTrackID].audioSamples[i],
												beginBufferRange, endBufferRange);
						this->sendVolumeData(bufferTrackID,	i);
						Networking::busyWait(100);
					}
					Networking::busyWait(10000);

					//Repost the buffering request until all packets have been buffered.
					if (endBufferRange < this->tracks[bufferTrackID].audioSamples.size()){
						PlaybackServerRequestData bufferRequestData;
						bufferRequestData.bufferInfo.trackID			= bufferTrackID;
						bufferRequestData.bufferInfo.beginBufferRange	= endBufferRange;
						bufferRequestData.bufferInfo.endBufferRange		= endBufferRange + Networking::ContinuousBufferCount;

						if (bufferRequestData.bufferInfo.endBufferRange > tracks[bufferTrackID].audioSamples.size())
							bufferRequestData.bufferInfo.endBufferRange = tracks[bufferTrackID].audioSamples.size();

						this->queueRequest(PlaybackServerRequestCode::PlaybackServer_BUFFER, bufferRequestData);
					}

					break;
					
				case PlaybackServerRequestCodes::PlaybackServer_PLAY:
					//If the current state is STOPPED, we have to do initial buffer, otherwise, we just send PLAY packets.
					//Otherwise otherwise, we just ignore it.
					if (this->playbackState == PlaybackStates::Playback_STOPPED) {
						
						//Send the first RequiredBufferedSamplesCount samples and accompanying volume information
						//and ensure that they arrive.
						for (unsigned int i = 0; i < tracks.size(); ++i) 
						{
						
							//Calculate the volume info for each track
							this->calculateVolumeData(i);

							//Playing from the beginning of the track implies that the remaining amount of time on a track is the entire length of that track
							tracks[i].playbackRemainingTime = tracks[i].trackLength;

							//If there are less samples in the track than the required initial buffering amount, buffer the entire
							//track.
							unsigned int trackBufferSize = min(tracks[i].audioSamples.size(), RequiredBufferedSamplesCount);

							//Add each sample as "not needing to be resent"
							for (unsigned int j = 0; j < trackBufferSize; ++j) {
								this->sendAudioSample(i, tracks[i].audioSamples[j], 0, trackBufferSize);
								//this->sendVolumeData(bufferTrackID,	i);
							}

							if (trackBufferSize == Networking::RequiredBufferedSamplesCount){
								PlaybackServerRequestData bufferRequestData;
								bufferRequestData.bufferInfo.trackID			= i;
								bufferRequestData.bufferInfo.beginBufferRange	= trackBufferSize;
								bufferRequestData.bufferInfo.endBufferRange		= trackBufferSize + Networking::ContinuousBufferCount;

								if (bufferRequestData.bufferInfo.endBufferRange > tracks[i].audioSamples.size())
									bufferRequestData.bufferInfo.endBufferRange = tracks[bufferTrackID].audioSamples.size();

								this->queueRequest(PlaybackServerRequestCode::PlaybackServer_BUFFER, bufferRequestData);
							}

						}

						//Send PLAY control packets

						playMessage.ControlByte = ControlBytes::BEGIN_PLAYBACK;

						//Indicate that playback should begin 3 x The set timeout for resending packets, giving the clients
						//approximately two chances to have their play controls dropped.
						//playMessage.TransportControl.timeOffset = getMicroseconds() + 3 * Networking::ClientReceivedPacketTimeout;
						playMessage.TransportControl.timeOffset = time(NULL) + 5;

						//Mark when each track began playback
						for (unsigned int i = 0; i < tracks.size(); ++i) {
							tracks[i].playbackOriginOffset = playMessage.TransportControl.timeOffset;
						}
					
						//Broadcast the message
						this->broadcastMessage(&playMessage, sizeof(playMessage));
						
						
						this->playbackState = PlaybackState::Playback_PLAYING;
					}
					else if (this->playbackState == PlaybackStates::Playback_PAUSED) {

						//Sync up on resume
						//Buffering should always be happening after the initial buffering, so no buffering needs to happen here
						
						//Send PLAY control packets

						playMessage.ControlByte = ControlBytes::BEGIN_PLAYBACK;

						//Indicate that playback should begin 3 x The set timeout for resending packets, giving the clients
						//approximately two chances to have their play controls dropped.
						//playMessage.TransportControl.timeOffset = getMicroseconds() + 3 * Networking::ClientReceivedPacketTimeout;
						playMessage.TransportControl.timeOffset = 0; //time(NULL) + 5;

						//Update when each track began playback again
						for (unsigned int i = 0; i < tracks.size(); ++i) {
							tracks[i].playbackOriginOffset = playMessage.TransportControl.timeOffset;
						}
					
						//Broadcast the message
						this->broadcastMessage(&playMessage, sizeof(playMessage));
					
						this->playbackState = PlaybackState::Playback_PLAYING;

					}
					
					break;

					//Pauses playback
				case PlaybackServerRequestCodes::PlaybackServer_PAUSE:

					//If we aren't currently playing, ignore this request.
					if (this->playbackState != PlaybackStates::Playback_PLAYING) break;

					//Send PAUSE control packets

					transportControlRequest.ControlByte = ControlBytes::PAUSE_PLAYBACK;

					//Indicate that the clients should pause immediately when they receive the message
					transportControlRequest.TransportControl.timeOffset = getMicroseconds();

					//Update the remaining amount of time for each track
					for (unsigned int i = 0; i < tracks.size(); ++i) {
						tracks[i].playbackRemainingTime = tracks[i].trackLength - (transportControlRequest.TransportControl.timeOffset - tracks[i].playbackOriginOffset);
					}
					
					//Broadcast the message
					this->broadcastMessage(&transportControlRequest, sizeof(transportControlRequest));

					this->playbackState = PlaybackStates::Playback_PAUSED;

					break;

					//Stops playback
				case PlaybackServerRequestCodes::PlaybackServer_STOP:
					
					//If we aren't currently playing, ignore this request.
					if (this->playbackState != PlaybackStates::Playback_PLAYING) break;

					//Send PAUSE control packets
					transportControlRequest.ControlByte = ControlBytes::STOP_PLAYBACK;

					//Indicate that the clients should pause immediately when they receive the message
					transportControlRequest.TransportControl.timeOffset = getMicroseconds();
					
					//Broadcast the message
					this->broadcastMessage(&transportControlRequest, sizeof(transportControlRequest));

					this->playbackState = PlaybackStates::Playback_STOPPED;

					break;

					//Load a new track
				case PlaybackServerRequestCodes::PlaybackServer_NEW_TRACK:
					
					TrackInfo newTrack;
					newTrack.TrackID = this->tracks.size();
					
					//Read the audio file
					int audioCode = this->readAudioDataFromFile(request.controlData.newTrackInfo.audioFilename, newTrack.audioSamples);
					//Read the position file
					int positionCode = this->readPositionDataFromFile(request.controlData.newTrackInfo.positionFilename, newTrack.audioSamples.size(), newTrack.positionData);

					//Set the length of the track
					newTrack.trackLength = newTrack.audioSamples.size() * 100000;

					newTrack.fileSize = audioCode;

					if (request.controlData.newTrackInfo.callback)
						request.controlData.newTrackInfo.callback(audioCode, positionCode, request.controlData.newTrackInfo.token);

					this->tracks[newTrack.TrackID] = newTrack;

					break;
				}

			} while (this->state != PlaybackServerStates::PlaybackServer_STOPPED);
		}

		///<summary>Multithreaded router function that calls serverMain().</summary>
		DWORD PlaybackServer::serverRouteMain(void* server){
			static_cast<PlaybackServer*>(server)->serverMain();
			return 0;
		}
		
		///<summary>Produces server timer-tick events.</summary>
		void PlaybackServer::timerTickEvent() {

			//Timer has started up.
			EnterCriticalSection(&this->timerDestroyedCriticalSection);
			
			while (this->state != PlaybackServerStates::PlaybackServer_INVALID) {
				//Queue up timer tick requests
				if (!this->timerTicked && this->state == PlaybackServerStates::PlaybackServer_RUNNING) {
					this->timerTicked = true;
					this->queueRequest(PlaybackServerRequestCodes::PlaybackServer_TIMER_TICK);
				}

				Sleep(200);
			}

			//Timer has ended.
			LeaveCriticalSection(&this->timerDestroyedCriticalSection);
		}

		///<summary>Multithreaded router function that calls timerTickEvent().</summary>
		DWORD PlaybackServer::timerRoute(void* server) {
			static_cast<PlaybackServer*>(server)->timerTickEvent();
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
			
			//Don't queue requests while the server is in a non-functional state.
			if (this->state != PlaybackServerStates::PlaybackServer_RUNNING)
				return;

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
			this->serverSocket = NULL;
			this->serverMainThread = NULL;
			this->serverReceivingThread = NULL;
			this->state = PlaybackServerStates::PlaybackServer_STOPPED;
			this->playbackState = PlaybackStates::Playback_INVALID;
			InitializeCriticalSection(&this->controlMessagesCriticalSection);
			InitializeCriticalSection(&this->timerDestroyedCriticalSection);
			this->controlMessagesResourceCount = CreateSemaphore(NULL, 0, USHRT_MAX, NULL);
			//Create timer thread
			this->timerTicked = false;
			CreateThread(NULL, 0, &PlaybackServer::timerRoute, (void*)this, 0, NULL);
		}

		///<summary>Destructs a PlaybackServer object.</summary>
		PlaybackServer::~PlaybackServer(){
			if (this->serverSocket){
				free(this->serverSocket);
				this->serverSocket = NULL;
			}

			if (this->serverMainThread)
				CloseHandle(this->serverMainThread);
			if (this->serverReceivingThread)
				CloseHandle(this->serverReceivingThread);
			CloseHandle(this->controlMessagesResourceCount);

			//Give the media timer a chance to quit.
			this->state = PlaybackServerStates::PlaybackServer_INVALID;
			EnterCriticalSection(&this->timerDestroyedCriticalSection);
			LeaveCriticalSection(&this->timerDestroyedCriticalSection);
			
			DeleteCriticalSection(&this->timerDestroyedCriticalSection);

			DeleteCriticalSection(&this->controlMessagesCriticalSection);
		}

		// ---------------------------------------------
		// PUBLIC METHODS
		// ---------------------------------------------
		
		///<summary>Attempts to start the PlaybackServer.</summary>
		///<returns>A PlaybackServerErrorCode indicating the result of this call.</returns>
		PlaybackServerErrorCode PlaybackServer::ServerStart(){
			
			Logger::openLogFile();

			//god I hate the stl
			std::queue<PlaybackServerRequest> emptyMessageQueue;

			switch(this->state){
			case PlaybackServerStates::PlaybackServer_RUNNING:
				//If the server is already running, do nothing.
				return PlaybackServerErrorCodes::PlaybackServer_OK;
				break;
			case PlaybackServerStates::PlaybackServer_STOPPED:

				//In case the server was running before and was stopped,
				//make sure that the threads had a chance to terminate.
				
			//================================================================================
				//The serverMain() SHOULD have stopped in order to arrive at this point.
				
				//If serverMain() has not stopped, attempt to signal it to terminate
				if (this->serverMainThread && WaitForSingleObject(this->serverMainThread, 1) == WAIT_TIMEOUT){
					this->queueRequest(PlaybackServerRequestCodes::PlaybackServer_STOP);

					//Wait for serverMain() to terminate. (do a join) ---------------------------------------- We could throw an exception here after a certain amount of time?
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
				
				return PlaybackServerErrorCodes::PlaybackServer_OK;

				break;
			}

			//The server is not in a valid state.
			return PlaybackServerErrorCodes::PlaybackServer_ISINVALID;
		}
		
		///<summary>Attempts to start playback.</summary>
		///<returns>A PlaybackErrorCode indicating the result of this call.</returns>
		PlaybackServerErrorCode PlaybackServer::Play(){

			Logger::logNotice("Attempting to set playback to 'PLAY'..");

			if (this->state != PlaybackServerStates::PlaybackServer_RUNNING) 
				return PlaybackServerErrorCodes::PlaybackServer_ISINVALID;

			this->queueRequest(PlaybackServerRequestCodes::PlaybackServer_PLAY);

			return PlaybackServerErrorCodes::PlaybackServer_OK;
		}
		
		///<summary>Attempts to pause playback.</summary>
		///<returns>A PlaybackErrorCode indicating the result of this call.</returns>
		PlaybackServerErrorCode PlaybackServer::Pause() {
			
			Logger::logNotice("Attempting to set playback to 'PAUSE'..");
			
			if (this->state != PlaybackServerStates::PlaybackServer_RUNNING) 
				return PlaybackServerErrorCodes::PlaybackServer_ISINVALID;

			this->queueRequest(PlaybackServerRequestCodes::PlaybackServer_PAUSE);

			return PlaybackServerErrorCodes::PlaybackServer_OK;
		}

		///<summary>Attempts to stop playback.</summary>
		///<returns>A PlaybackErrorCode indicating the result of this call.</returns>
		PlaybackServerErrorCode PlaybackServer::Stop() {
			
			Logger::logNotice("Attempting to set playback to 'STOP'..");

			if (this->state != PlaybackServerStates::PlaybackServer_RUNNING) 
				return PlaybackServerErrorCodes::PlaybackServer_ISINVALID;

			this->queueRequest(PlaybackServerRequestCodes::PlaybackServer_STOP);

			return PlaybackServerErrorCodes::PlaybackServer_OK;
		}

		///<summary>Attempts to create a PlaybackServer instance and returns an error code if it could not. fillServer is filled with NULL if creation fails.</summary>
		///<param name="fillServer">A reference to the PlaybackServer object to fill.</param>
		///<returns>A Networking::SocketErrorCode.</returns>
		int PlaybackServer::Create(PlaybackServer** fillServer){

			WSADATA wsaData;
			WSAStartup(MAKEWORD(2,2), &wsaData);
			
			//Check the pointer, and initialize to NULL
			if (!fillServer) return Networking::SocketErrorCode::SocketError_POINTER;
			*fillServer = NULL;

			PlaybackServer* server = new PlaybackServer();

			//Attempt to create the sendSocket inside the Server
			server->serverSocket = sockets_server_socket(Networking::NetworkPort);
			if (!server->serverSocket)
				return errno;

			*fillServer = server;
			return Networking::SocketErrorCode::SocketError_OK;
		}

		///<summary>Attemps to add a track to the server's playlist.</summary>
		///<param name="audioFilename">The name of the audio file to add.</param>
		///<param name="positionFilename">The name of the corresponding position data file.</param>
		///<returns>A PlaybackServerErrorCode indicating the result of this call.</returns>
		PlaybackServerErrorCode PlaybackServer::AddTrack(char* audioFilename, char* positionFilename){
			return this->AddTrack(audioFilename, positionFilename, NULL, 0);
		}

		///<summary>Queues up a request to asynchronously add a track to the server's playlist.</summary>
		///<param name="audioFilename">The name of the audio file to add.</param>
		///<param name="positionFilename">The name of the corresponding position data file.</param>
		///<param name="callback">The callback function to call when the corresponding request completes.</param>
		///<param name="token">An identifying token that is passed along with the callback when the corresponding request complete.</param>
		///<returns>A PlaybackServerErrorCode indicating the result of this call.</returns>
		PlaybackServerErrorCode PlaybackServer::AddTrack(char* audioFilename, char* positionFilename, AddTrackCallback callback, uint64_t token){
			
			Logger::logNotice("Attempting to add new track:=; audio file %s; position file %s", audioFilename, positionFilename);
			
			if (this->state != PlaybackServerStates::PlaybackServer_RUNNING) 
				return PlaybackServerErrorCodes::PlaybackServer_ISINVALID;

			//Queue up a track.
			PlaybackServerRequestData data;
			strcpy(data.newTrackInfo.audioFilename, audioFilename);
			strcpy(data.newTrackInfo.positionFilename, positionFilename);
			data.newTrackInfo.callback = callback;
			data.newTrackInfo.token = token;
			this->queueRequest(PlaybackServerRequestCodes::PlaybackServer_NEW_TRACK, data);

			return PlaybackServerErrorCodes::PlaybackServer_OK;
		}

		///<summary>Subscribes the caller to the ClientConnected event. ClientConnected is raised when a new client appears on the network.</summary>
		///<param name="callback">A pointer to the function to call when the event is raised.</param>
		void PlaybackServer::OnClientConnected(ClientConnectedCallback callback) {
			this->clientConnectedCallbacks.push_back(callback);
		}

		///<summary>Subscribes the caller to the ClientDisconnected event. ClientDisconnected is raised when a client disconnects from the network.</summary>
		///<param name="callback">A pointer to the function to call when the event is raised.</param>
		void PlaybackServer::OnClientDisconnected(ClientDisconnectedCallback callback) {
			this->clientDisconnectedCallbacks.push_back(callback);
		}

}