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
		void PlaybackServer::dispatchNetworkMessage(char* datagram) {

		}
		
		// ---------------------------------------------
		// PUBLIC METHODS
		// ---------------------------------------------
		
		///<summary>Attempts to start the PlaybackServer.</summary>
		///<returns>Integer error code on failure, 0 on success.</returns>
		int PlaybackServer::Start(){

			while (1){
				
				char datagram[1500];
				int grams = this->socket->ReceiveMessage(datagram, 1500);

				switch (datagram[0]){
				case Networking::NEW_CONNECTION:
					receiveClientConnection(datagram, grams);
					break;
				}

			}

			return PlaybackServerCodes::PlaybackServer_OK;

		}
		
		///<summary>Empty default constructor.</summary>
		PlaybackServer::PlaybackServer() {
			this->socket = NULL;
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