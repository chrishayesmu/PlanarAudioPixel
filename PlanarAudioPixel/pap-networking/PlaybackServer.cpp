#include "PlaybackServer.h"
#include "Socket.h"

namespace Networking {
	
		///<summary>Receives information from a client and stores it in the client information list.</summary>
		///<param name="data">The datagram data.</param>
		///<param name="dataSize">The number of bytes in the datagram.</param>
		void PlaybackServer::receiveClientConnection(char* data, int dataSize) {

		}

		///<summary>Creates a synchronization thread that will maintain system synchronization.</summary>
		void PlaybackServer::createSyncThread() {

		}

		///<summary>The entry point for the syncronization threads. Handles syncronizing the clients.</summary>
		void PlaybackServer::handleSyncThread() {

		}

		///<summary>Send the server’s timestamp to all of its clients.</summary>
		void PlaybackServer::broadcastSyncMessage() {

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

		///<summary>Spawns a new thread to process audio and positional data. </summary>
		///<param name="audioFilename">The name of the audio file.</summary>
		///<param name="positionFilename">The name of the position information data file.</summary>
		void PlaybackServer::processAudioFile(char* audioFilename, char* positionFilename) {

		}

		///<summary>Reads audio data from the file specified and returns it as an array.</summary>
		///<param name="filename">The name of the audio file.</param>
		///<returns>Audio information objects as an array.</returns>
		AudioSample* PlaybackServer::readAudioDataFromFile(char* filename) {

		}

		///<summary>Reads position data from the file specified and returns it as an array.</summary>
		///<param name="filename">The name of the position data file.</param>
		///<returns>Volume information data as an array.</returns>
		VolumeInfo* PlaybackServer::readPositionDataFromFile(char* filename) {

		}

		///<summary>Broadcasts an audio sample to the client network.</summary>
		///<param name="sampleBuffer">The sample data.</param>
		///<param name="sampleSize">The size of the sample data.</param>
		///<returns>Integer return code specifying the result of the call.</returns>
		int PlaybackServer::sendAudioSample(AudioSample sampleBuffer, int sampleSize) {

		}

		///<summary>Broadcasts volume information for the client network.</summary>
		///<param name="segmentID">The ID of the segment to which this bit of volume data applies.</param>
		///<param name="trackID">The ID of the track to which this bit of volume data applies.</param>
		///<param name="volumeDataBuffer">The raw data containing the volume information.</param>
		///<param name="bufferSize">The number of bytes volume data.</param>
		///<returns>Integer return code specifying the result of the call.</returns>
		int PlaybackServer::sendVolumeData(int segmentID, int trackID, char* volumeDataBuffer, int bufferSize) {

		}

		///<summary>Single entry point for all network communications. Reads the control byte and acts on it accordingly.</summary>
		///<param name="datagram">The datagram data.</param>
		void PlaybackServer::dispatchNetworkMessage(char* datagram) {

		}


		void PlaybackServer::testStart(){

		}
}