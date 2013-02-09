#include "PlaybackServer.h"
#include "Socket.h"
#include "NetworkGlobals.h"
#include "ControlByteConstants.h"

namespace Networking {
		
		///<summary>Receives information from a client and stores it in the client information list.</summary>
		///<param name="data">The datagram data.</param>
		///<param name="dataSize">The number of bytes in the datagram.</param>
		void PlaybackServer::receiveClientConnection(char* data, int dataSize) {

		}

		///<summary>Responds to a delay request sent by a client.</summary>
		///<param name="clientID">The ID of the client to send the response to.</param>
		void PlaybackServer::sendDelayResponseMessage(ClientGUID clientID) {

		}

		///<summary>Responds to an audio data resend request.</summary>
		///<param name="data">The datagram data.</summary>
		///<param name="dataSize">The number of bytes in the datagram.</param>
		void PlaybackServer::resendAudio(char* data, int dataSize) {

		}

		///<summary>Updates the client information table for the client that sent the check in.</summary>
		///<param name="data">The datagram data.</summary>
		///<param name="dataSize">The number of bytes in the datagram.</param>
		void PlaybackServer::receiveClientCheckIn(char* data, int dataSize) {

		}

		///<summary>This function will be called in order to notify the server that one or more clients have moved, join, or dropped out, and that the volume must therefore be recalculated.</summary>
		void PlaybackServer::clientPositionsChanged() {

		}

		///<summary>Spawns a new thread to process audio and positional data. The thread will 
		/// automatically terminate itself when finished.</summary>
		///<param name="audioFilename">The name of the audio file.</summary>
		///<param name="positionFilename">The name of the position information data file.</summary>
		void PlaybackServer::processAudioFilesOnThread(char* audioFilename, char* positionFilename) {

		}

		///<summary>Reads audio data from the file specified and returns it as an array.</summary>
		///<param name="filename">The name of the audio file.</param>
		///<param name="data">A pointer which will be filled with the address of the audio data array.</param>
		///<param name="size">A pointer which will be filled with the number of elements in the audio data array.</param>
		///<returns>TODO: Integer return code specifying the result of the call.</returns>
		int PlaybackServer::readAudioDataFromFile(char* filename, AudioSample** data, int* size) {

			return NULL;
		}

		///<summary>Reads position data from the file specified and returns it as an array.</summary>
		///<param name="filename">The name of the position data file.</param>
		///<param name="data">A pointer which will be filled with the address of the position data array.</param>
		///<param name="size">A pointer which will be filled with the number of elements in the position data array.</param>
		///<returns>TODO: Integer return code specifying the result of the call.</returns>
		int PlaybackServer::readPositionDataFromFile(char* filename, PositionInfo** data, int* size) {

			return NULL;
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
			
		}
		
		///<summary>Handles network communications and hands off incoming packets to dispatchNetworkMessage().</summary>
		void PlaybackServer::serverReceive(){

		}
		///<summary>Multithreaded router function that calls serverReceieve().</summary>
		DWORD PlaybackServer::serverRouteReceive(void* server){
			static_cast<PlaybackServer*>(server)->serverReceive();
			return 0;
		}
		
		///<summary>Handles server management and control messages.</summary>
		void PlaybackServer::serverMain(){

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
			case PlaybackServerStates::PlaybackServer_PAUSED:

				//Allow each thread to unpause.
				ReleaseSemaphore(this->pausedStateSem, 2, NULL);
				return PlaybackServerStates::PlaybackServer_RUNNING;

				break;
			}

			//The server is not in a valid state.
			return PlaybackServerErrorCodes::PlaybackServer_INVALID;
		}
		
		///<summary>Constructs a PlaybackServer object.</summary>
		PlaybackServer::PlaybackServer() {
			this->socket = NULL;
			this->serverMainThread = NULL;
			this->serverReceivingThread = NULL;
			this->state = PlaybackServerStates::PlaybackServer_STOPPED;
			InitializeCriticalSection(&this->controlMessagesCriticalSection);
			this->controlMessagesResourceCount = CreateSemaphore(NULL, 0, USHRT_MAX, NULL);
			this->pausedStateSem = CreateSemaphore(NULL, 0, USHRT_MAX, NULL);
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
			CloseHandle(this->pausedStateSem);

			DeleteCriticalSection(&this->controlMessagesCriticalSection);
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