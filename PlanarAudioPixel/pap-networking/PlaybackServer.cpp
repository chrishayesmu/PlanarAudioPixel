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
			
			//If the client doesn't exist, fire the ClientConnected event
			if (Networking::ClientInformationTable.find(client.ClientID) == Networking::ClientInformationTable.end()) {
				for (int i = 0; i < this->clientConnectedCallbacks.size(); ++i) {
					this->clientConnectedCallbacks[i](client);
				}
			}

			//Add the client to the information table
			Networking::ClientInformationTable[client.ClientID] = client;

		}

		///<summary>Updates the client information table for the client that sent the check in.</summary>
		///<param name="message">The message data.</param>
		///<param name="dataSize">The number of bytes in the datagram.</param>
		void PlaybackServer::receiveClientCheckIn(const PacketStructures::NetworkMessage* message, int dataSize) {
			
			//Update the timestamp
			Networking::ClientInformationTable[message->ClientConnection.clientID].LastCheckInTime = getMicroseconds();

			//Raise the ClientCheckIn event
			for (int i = 0; i < this->clientCheckInCallbacks.size(); ++i) {
				this->clientCheckInCallbacks[i](Networking::ClientInformationTable[message->ClientConnection.clientID]);
			}

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

			//Drop resend requests that appear within a timeout's worth of microseconds of each other
			ResendRequestIterator c = this->sampleResendRequests.find(message->AudioResendRequest.TrackID);
			if (c != this->sampleResendRequests.end())
			{
				if (c->second.find(message->AudioResendRequest.SampleID) != c->second.end() &&
					getMicroseconds() - c->second[message->AudioResendRequest.SampleID] < Networking::ClientReceivedPacketTimeout) 
				{
					return;
				}
			}

			//Resend the sample
			this->sendAudioSample(message->AudioResendRequest.TrackID,
									this->tracks[message->AudioResendRequest.TrackID].audioSamples[message->AudioResendRequest.SampleID],
									message->AudioResendRequest.BufferRangeStartID,
									message->AudioResendRequest.BufferRangeEndID);
			
			//Mark the last time this resend request was received
			(this->sampleResendRequests[message->AudioResendRequest.TrackID])[message->AudioResendRequest.SampleID] = getMicroseconds();

		}

		///<summary>Responds to a volume data resend request.</summary>
		///<param name="message">The message data.</summary>
		///<param name="dataSize">The number of bytes in the datagram.</param>
		void PlaybackServer::resendVolume(const PacketStructures::NetworkMessage* message, int dataSize){
			
			//Drop resend requests that appear within a timeout's worth of microseconds of each other
			ResendRequestIterator c = this->volumeResendRequests.find(message->VolumeResendRequest.TrackID);
			if (c != this->volumeResendRequests.end())
			{
				if (c->second.find(message->VolumeResendRequest.SampleID) != c->second.end() &&
					getMicroseconds() - c->second[message->VolumeResendRequest.SampleID] < Networking::ClientReceivedPacketTimeout) 
				{
					return;
				}
			}

			//Resend the volume data
			this->sendVolumeData(message->VolumeResendRequest.TrackID, message->VolumeResendRequest.SampleID,
									this->tracks[message->VolumeResendRequest.TrackID].volumeData[message->VolumeResendRequest.SampleID],
									message->VolumeResendRequest.BufferRangeStartID,
									message->VolumeResendRequest.BufferRangeEndID);
			
			//Mark the last time this resend request was received
			(this->volumeResendRequests[message->VolumeResendRequest.TrackID])[message->VolumeResendRequest.SampleID] = getMicroseconds();

		}

		///<summary>This function will be called in order to notify the server that one or more clients have moved, join, or dropped out, and that the volume must therefore be recalculated.</summary>
		void PlaybackServer::clientPositionsChanged() {

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
		void PlaybackServer::sendAudioSample(trackid_t trackID, AudioSample sampleBuffer, sampleid_t bufferRangeStartID, sampleid_t bufferRangeEndID){

			//Define a struct capable of containing both the network message and the buffer data.
			struct {
				PacketStructures::NetworkMessage networkHeader;
				char data[1500-32];
			} audioSampleMessage;

			audioSampleMessage.networkHeader.ControlByte = ControlBytes::SENDING_AUDIO;
			audioSampleMessage.networkHeader.AudioSample.SampleID = sampleBuffer.SampleID;
			audioSampleMessage.networkHeader.AudioSample.TrackID = trackID;
			audioSampleMessage.networkHeader.AudioSample.BufferRangeStartID = bufferRangeStartID;
			audioSampleMessage.networkHeader.AudioSample.BufferRangeEndID = bufferRangeEndID;

			//Assume that the function constructing the sample buffer behaved well and didn't produce a sample with greater than 1468 bytes.
			memcpy(audioSampleMessage.data, sampleBuffer.Data.Data, sampleBuffer.Data.DataLength);

			//Include the length of the data in this message.
			audioSampleMessage.networkHeader.Extra._dataLength = sampleBuffer.Data.DataLength;

			//Send the data
			this->sendSocket->SendMessage((char*)&audioSampleMessage, sizeof(PacketStructures::NetworkMessage) + sampleBuffer.Data.DataLength);

		}

		///<summary>Broadcasts volume information for the client network.</summary>
		///<param name="trackID">The ID of the track to which this bit of volume data applies.</param>
		///<param name="sampleID">The ID of the sample to which this bit of volume data applies.</param>
		///<param name="bufferRangeStartID">The ID of the first sample in the buffering range.</param>
		///<param name="bufferRangeEndID">The ID of the last sample in the buffering range.</param>
		///<returns>Integer return code specifying the result of the call.</returns>
		void PlaybackServer::sendVolumeData(trackid_t trackID, sampleid_t sampleID, VolumeInfo volumeData, sampleid_t bufferRangeStartID, sampleid_t bufferRangeEndID) {
						
			//Define a struct capable of containing both the network message and 121 clients of volume data.
			struct {
				PacketStructures::NetworkMessage networkHeader;
				PacketStructures::ClientVolume volumeData[121];
			} volumeDataMessage;

			volumeDataMessage.networkHeader.ControlByte = ControlBytes::SENDING_VOLUME;
			volumeDataMessage.networkHeader.AudioSample.SampleID = sampleID;
			volumeDataMessage.networkHeader.AudioSample.TrackID = trackID;
			volumeDataMessage.networkHeader.AudioSample.BufferRangeStartID = bufferRangeStartID;
			volumeDataMessage.networkHeader.AudioSample.BufferRangeEndID = bufferRangeEndID;

			//Include the number of clients contained in this volume data message.
			volumeDataMessage.networkHeader.Extra._dataLength = volumeData.size();

			//Index into volumeDataMessage.volumeData array.
			int j = 0;

			//Indicator for first/second client in current ClientVolume structure.
			int e = 0;
			for (VolumeInfoIterator i = volumeData.begin(); i != volumeData.end(); ++i) {
				if (e == 0){
					volumeDataMessage.volumeData[j].clientID_1 = i->first;
					volumeDataMessage.volumeData[j].clientVolume_1 = i->second;
					++e;
				} else {
					volumeDataMessage.volumeData[j].clientID_2 = i->first;
					volumeDataMessage.volumeData[j].clientVolume_2 = i->second;
					e = 0;
					++j;
				}
			}

			//Send the data
			this->sendSocket->SendMessage((char*)&volumeDataMessage, sizeof(PacketStructures::NetworkMessage) + sizeof(PacketStructures::ClientVolume) * volumeData.size());

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

				//Transport control acknowledgements
			case Networking::ControlBytes::BEGIN_PLAYBACK:
			case Networking::ControlBytes::STOP_PLAYBACK:
			case Networking::ControlBytes::PAUSE_PLAYBACK:
				
				//Acknowledge that this request has been filled by this client.
				(this->requestsAcknowledged[message->TransportControl.requestID])[message->TransportControl.clientID] = true;

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
			this->playbackState = PlaybackStates::Playback_STOPPED;

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
				int bufferMax = 0;
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

					if (this->playbackState == PlaybackStates::Playback_PLAYING) {

						//Check to see if all tracks have ended
						bool ended = true;
						for (int i = 0; i < tracks.size(); ++i){
							if ((getMicroseconds() - tracks[i].playbackRemainingTime) < tracks[i].playbackOriginOffset) {
								ended = false;
								break;
							}
						}
						if (ended) {
							//Tracks have ended - Send a stop request
							this->queueRequest(PlaybackServerRequestCode::PlaybackServer_STOP);
						}

					}

					//Check to see if any clients have timedout


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
						this->sendVolumeData(bufferTrackID,
												i, this->tracks[bufferTrackID].volumeData[i],
												beginBufferRange, endBufferRange);
					}
					Networking::busyWait(Networking::ClientReceivedPacketTimeout);

					//Repost the buffering request until all packets have been buffered.
					if (endBufferRange == Networking::RequiredBufferedSamplesCount){
						PlaybackServerRequestData bufferRequestData;
						bufferRequestData.bufferInfo.trackID			= bufferTrackID;
						bufferRequestData.bufferInfo.beginBufferRange	= endBufferRange;
						bufferRequestData.bufferInfo.endBufferRange		= endBufferRange + Networking::ContinuousBufferCount;

						if (bufferRequestData.bufferInfo.endBufferRange > tracks[bufferTrackID].audioSamples.size())
							bufferRequestData.bufferInfo.endBufferRange = tracks[bufferTrackID].audioSamples.size() - endBufferRange;

						this->queueRequest(PlaybackServerRequestCode::PlaybackServer_BUFFER, bufferRequestData);
					}

					break;
					
				case PlaybackServerRequestCodes::PlaybackServer_PLAY:
					//If the current state is STOPPED, we have to do initial buffer, otherwise, we just send PLAY packets.
					//Otherwise otherwise, we just ignore it.
					if (this->playbackState == PlaybackStates::Playback_STOPPED) {
						//Indicate that this is the initial buffering phase
						this->initialBuffering = true;

						//Acquire a local copy of the list of clients
						clients = ClientInformationTable;
					
						//Count the number of samples to buffer
						for (unsigned int i = 0; i < tracks.size(); ++i) {
							bufferMax += min(tracks[i].audioSamples.size(), RequiredBufferedSamplesCount) * 2;
						}

						//Send the first RequiredBufferedSamplesCount samples and accompanying volume information
						//and ensure that they arrive.
						for (unsigned int i = 0; i < tracks.size(); ++i) 
						{
							//Playing from the beginning of the track implies that the remaining amount of time on a track is the entire length of that track
							tracks[i].playbackRemainingTime = tracks[i].trackLength;

							//If there are less samples in the track than the required initial buffering amount, buffer the entire
							//track.
							unsigned int trackBufferSize = min(tracks[i].audioSamples.size(), RequiredBufferedSamplesCount);

							//Add each sample as "not needing to be resent"
							this->samplesResend.clear();
							this->volumesResend.clear();
							for (unsigned int j = 0; j < trackBufferSize; ++j) {
								this->samplesResend[j] = false;
								this->volumesResend[j] = false;
							}

							//Send samples
							do {
								//Iterate over the keys left in sampleReceivedCounts (those are the sampleid_t's of the samples that haven't successfully buffered),
								//and attempt to buffer them.
								for (BufferingIterator j = this->samplesResend.begin(); j != this->samplesResend.end(); ++j) {
									this->sendAudioSample(i, tracks[i].audioSamples[j->first], 0, trackBufferSize);
								}
														
								//Wait for the amount of time that should indicate that every client either recieved or did not recieve one of the packets
								Networking::busyWait(Networking::ClientReceivedPacketTimeout);

								std::map<sampleid_t, bool> tempSampleResend;

								//Add samples back that didn't make it
								for (BufferingIterator j = this->samplesResend.begin(); j != this->samplesResend.end(); ++j) {
									if (j->second)
										tempSampleResend[j->first] = false;
								}
								this->samplesResend = tempSampleResend;

							} while (this->samplesResend.size());

							//Send volume data
							do {
								//Iterate over the keys left in sampleReceivedCounts (those are the sampleid_t's of the samples that haven't successfully buffered),
								//and attempt to buffer them.
								for (BufferingIterator j = this->volumesResend.begin(); j != this->volumesResend.end(); ++j) {
									this->sendVolumeData(i, j->first, tracks[i].volumeData[j->first], 0, trackBufferSize);
								}
							
								//Wait for the amount of time that should indicate that every client either recieved or did not recieve one of the packets
								Networking::busyWait(Networking::ClientReceivedPacketTimeout);

								std::map<sampleid_t, bool> tempVolumeResend;

								//Add samples back that didn't make it
								for (BufferingIterator j = this->volumesResend.begin(); j != this->volumesResend.end(); ++j) {
									if (j->second)
										tempVolumeResend[j->first] = false;
								}
								this->samplesResend = tempVolumeResend;

							} while (this->samplesResend.size());

							if (trackBufferSize == Networking::RequiredBufferedSamplesCount){
								PlaybackServerRequestData bufferRequestData;
								bufferRequestData.bufferInfo.trackID			= i;
								bufferRequestData.bufferInfo.beginBufferRange	= trackBufferSize;
								bufferRequestData.bufferInfo.endBufferRange		= trackBufferSize + Networking::ContinuousBufferCount;

								if (bufferRequestData.bufferInfo.endBufferRange > tracks[i].audioSamples.size())
									bufferRequestData.bufferInfo.endBufferRange = tracks[i].audioSamples.size() - trackBufferSize;

								this->queueRequest(PlaybackServerRequestCode::PlaybackServer_BUFFER, bufferRequestData);
							}

						}

						//Buffering is complete
						this->initialBuffering = false;

						//Send PLAY control packets and ensure that every client has begun playback
						++this->currentRequestID;

						playMessage.ControlByte = ControlBytes::BEGIN_PLAYBACK;
						playMessage.TransportControl.requestID = this->currentRequestID;
						
						//Indicate that this is a PLAY operation, and that the clients should begin playback at the specified offset.
						playMessage.Extra._extra = -1;

						//Specify the offset at 0 for initial playback.
						playMessage.TransportControl.timeOffset = 0;

						//Indicate that playback should begin 3 x The set timeout for resending packets, giving the clients
						//approximately two chances to have their play controls dropped.
						playMessage.TransportControl.timeOffset = getMicroseconds() + 3 * Networking::ClientReceivedPacketTimeout;

						//Mark when each track began playback
						for (int i = 0; i < tracks.size(); ++i) {
							tracks[i].playbackOriginOffset = playMessage.TransportControl.timeOffset;
						}
					
						for (int i = 0; i < 3 && resend; ++i){

							this->sendSocket->SendMessage((char*)&playMessage, sizeof(playMessage));

							//Wait for the amount of time that should indicate that every client either recieved or did not recieve one of the packets
							Networking::busyWait(Networking::ClientReceivedPacketTimeout);

							resend = false;
							//Iterate over the acknowledged requests to determine if any clients did not receive the play control byte
							for (ClientAcknowledgementIterator j = this->requestsAcknowledged[this->currentRequestID].begin(); j != this->requestsAcknowledged[this->currentRequestID].end(); ++j)
								if (j->second) {
									resend = true;
									break;
								}
						}
					
						this->playbackState = PlaybackState::Playback_PLAYING;
					}
					else if (this->playbackState == PlaybackStates::Playback_PAUSED) {

						//Sync up on resume
						//Buffering should always be happening after the initial buffering, so no buffering needs to happen here
						
						//Send PLAY control packets and ensure that every client has begun playback
						++this->currentRequestID;

						playMessage.ControlByte = ControlBytes::BEGIN_PLAYBACK;
						playMessage.TransportControl.requestID = this->currentRequestID;

						//Indicate that this is a RESUME operation.
						playMessage.Extra._extra = -1;

						//Indicate that playback should begin 3 x The set timeout for resending packets, giving the clients
						//approximately two chances to have their play controls dropped.
						playMessage.TransportControl.timeOffset = getMicroseconds() + 3 * Networking::ClientReceivedPacketTimeout;

						//Update when each track began playback again
						for (int i = 0; i < tracks.size(); ++i) {
							tracks[i].playbackOriginOffset = playMessage.TransportControl.timeOffset;
						}
					
						for (int i = 0; i < 3 && resend; ++i){

							this->sendSocket->SendMessage((char*)&playMessage, sizeof(playMessage));

							//Wait for the amount of time that should indicate that every client either recieved or did not recieve one of the packets
							Networking::busyWait(Networking::ClientReceivedPacketTimeout);

							resend = false;
							//Iterate over the acknowledged requests to determine if any clients did not receive the play control byte
							for (ClientAcknowledgementIterator j = this->requestsAcknowledged[this->currentRequestID].begin(); j != this->requestsAcknowledged[this->currentRequestID].end(); ++j)
								if (j->second) {
									resend = true;
									break;
								}
						}
					
						this->playbackState = PlaybackState::Playback_PLAYING;

					}
					
					break;

					//Pauses playback
				case PlaybackServerRequestCodes::PlaybackServer_PAUSE:

					//If we aren't currently playing, ignore this request.
					if (this->playbackState != PlaybackStates::Playback_PLAYING) break;

					//Send PAUSE control packets and ensure that every client has paused playback
					++this->currentRequestID;

					transportControlRequest.ControlByte = ControlBytes::PAUSE_PLAYBACK;
					transportControlRequest.TransportControl.requestID = this->currentRequestID;

					//Indicate that the clients should pause immediately when they receive the message
					transportControlRequest.TransportControl.timeOffset = getMicroseconds();

					//Update the remaining amount of time for each track
					for (int i = 0; i < tracks.size(); ++i) {
						tracks[i].playbackRemainingTime = tracks[i].trackLength - (transportControlRequest.TransportControl.timeOffset - tracks[i].playbackOriginOffset);
					}
					
					do {

						this->sendSocket->SendMessage((char*)&transportControlRequest, sizeof(transportControlRequest));

						//Wait for the amount of time that should indicate that every client either recieved or did not recieve one of the packets
						Networking::busyWait(Networking::ClientReceivedPacketTimeout);

						resend = false;
						//Iterate over the acknowledged requests to determine if any clients did not receive the play control byte
						for (ClientAcknowledgementIterator j = this->requestsAcknowledged[this->currentRequestID].begin(); j != this->requestsAcknowledged[this->currentRequestID].end(); ++j)
							if (j->second) {
								resend = true;
								break;
							}

					} while (resend);

					this->playbackState = PlaybackStates::Playback_PAUSED;

					break;

					//Stops playback
				case PlaybackServerRequestCodes::PlaybackServer_STOP:
					
					//If we aren't currently playing, ignore this request.
					if (this->playbackState != PlaybackStates::Playback_PLAYING) break;

					//Send PAUSE control packets and ensure that every client has paused playback
					++this->currentRequestID;

					transportControlRequest.ControlByte = ControlBytes::STOP_PLAYBACK;
					transportControlRequest.TransportControl.requestID = this->currentRequestID;

					//Indicate that the clients should pause immediately when they receive the message
					transportControlRequest.TransportControl.timeOffset = getMicroseconds();
					
					do {

						this->sendSocket->SendMessage((char*)&transportControlRequest, sizeof(transportControlRequest));

						//Wait for the amount of time that should indicate that every client either recieved or did not recieve one of the packets
						Networking::busyWait(Networking::ClientReceivedPacketTimeout);

						resend = false;
						//Iterate over the acknowledged requests to determine if any clients did not receive the play control byte
						for (ClientAcknowledgementIterator j = this->requestsAcknowledged[this->currentRequestID].begin(); j != this->requestsAcknowledged[this->currentRequestID].end(); ++j)
							if (j->second) {
								resend = true;
								break;
							}

					} while (resend);

					this->playbackState = PlaybackStates::Playback_STOPPED;

					break;

					//Load a new track
				case PlaybackServerRequestCodes::PlaybackServer_NEW_TRACK:
					
					TrackInfo newTrack;
					newTrack.TrackID = this->tracks.size() + 1;
					
					//Read the audio file
					int audioCode = this->readAudioDataFromFile(request.controlData.newTrackInfo.audioFilename, newTrack.audioSamples);
					//Read the position file
					int positionCode = this->readPositionDataFromFile(request.controlData.newTrackInfo.positionFilename, newTrack.positionData);

					//Set the length of the track
					newTrack.trackLength = newTrack.audioSamples.size() * 100000;

					if (request.controlData.newTrackInfo.callback)
						request.controlData.newTrackInfo.callback(audioCode, positionCode, request.controlData.newTrackInfo.token);

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
			this->recvSocket = NULL;
			this->sendSocket = NULL;
			this->serverMainThread = NULL;
			this->serverReceivingThread = NULL;
			this->currentRequestID = 0;
			this->state = PlaybackServerStates::PlaybackServer_STOPPED;
			this->playbackState = PlaybackStates::Playback_INVALID;
			this->initialBuffering = false;
			InitializeCriticalSection(&this->controlMessagesCriticalSection);
			InitializeCriticalSection(&this->timerDestroyedCriticalSection);
			this->controlMessagesResourceCount = CreateSemaphore(NULL, 0, USHRT_MAX, NULL);

			//Create timer thread
			this->timerTicked = false;
			CreateThread(NULL, 0, &PlaybackServer::timerRoute, (void*)this, 0, NULL);
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
			if (this->state != PlaybackServerStates::PlaybackServer_RUNNING) 
				return PlaybackServerErrorCodes::PlaybackServer_ISINVALID;

			this->queueRequest(PlaybackServerRequestCodes::PlaybackServer_PLAY);

			return PlaybackServerErrorCodes::PlaybackServer_OK;
		}
		
		///<summary>Attempts to pause playback.</summary>
		///<returns>A PlaybackErrorCode indicating the result of this call.</returns>
		PlaybackServerErrorCode PlaybackServer::Pause() {
			if (this->state != PlaybackServerStates::PlaybackServer_RUNNING) 
				return PlaybackServerErrorCodes::PlaybackServer_ISINVALID;

			this->queueRequest(PlaybackServerRequestCodes::PlaybackServer_PAUSE);

			return PlaybackServerErrorCodes::PlaybackServer_OK;
		}

		///<summary>Attempts to stop playback.</summary>
		///<returns>A PlaybackErrorCode indicating the result of this call.</returns>
		PlaybackServerErrorCode PlaybackServer::Stop() {
			if (this->state != PlaybackServerStates::PlaybackServer_RUNNING) 
				return PlaybackServerErrorCodes::PlaybackServer_ISINVALID;

			this->queueRequest(PlaybackServerRequestCodes::PlaybackServer_STOP);

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
		void PlaybackServer::OnClientDisconnected(ClientConnectedCallback callback) {
			this->clientDisconnectedCallbacks.push_back(callback);
		}

		///<summary>Subscribes the caller to the ClientCheckIn event. ClientCheckIn is raised when a client checks in to the network.</summary>
		///<param name="callback">A pointer to the function to call when the event is raised.</param>
		void PlaybackServer::OnClientCheckIn(ClientCheckInCallback callback) {
			this->clientCheckInCallbacks.push_back(callback);
		}
}