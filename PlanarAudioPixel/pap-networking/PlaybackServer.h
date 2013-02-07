#include "NetworkStructures.h"
//Giancarlo

namespace Networking {

	class PlaybackServer {
	private:

		///<summary>Receives information from a client and stores it in the client information list.</summary>
		///<param name="data">The datagram data.</param>
		///<param name="dataSize">The number of bytes in the datagram.</param>
		void receiveClientConnection(char* data, int dataSize);

		///<summary>Creates a synchronization thread that will maintain system synchronization.</summary>
		void createSyncThread();

		///<summary>The entry point for the syncronization threads. Handles syncronizing the clients.</summary>
		void handleSyncThread();

		///<summary>Send the server’s timestamp to all of its clients.</summary>
		void broadcastSyncMessage();

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

		///<summary>Spawns a new thread to process audio and positional data. </summary>
		///<param name="audioFilename">The name of the audio file.</summary>
		///<param name="positionFilename">The name of the position information data file.</summary>
		void processAudioFile(char* audioFilename, char* positionFilename);

		///<summary>Reads audio data from the file specified and returns it as an array.</summary>
		///<param name="filename">The name of the audio file.</param>
		///<returns>Audio information objects as an array.</returns>
		AudioSample* readAudioDataFromFile(char* filename);

		///<summary>Reads position data from the file specified and returns it as an array.</summary>
		///<param name="filename">The name of the position data file.</param>
		///<returns>Volume information data as an array.</returns>
		PositionInfo* readPositionDataFromFile(char* filename);

		///<summary>Broadcasts an audio sample to the client network.</summary>
		///<param name="sampleBuffer">The sample data.</param>
		///<param name="sampleSize">The size of the sample data.</param>
		///<returns>Integer return code specifying the result of the call.</returns>
		int sendAudioSample(AudioSample sampleBuffer, int sampleSize);

		///<summary>Broadcasts volume information for the client network.</summary>
		///<param name="sampleID">The ID of the sample to which this bit of volume data applies.</param>
		///<param name="trackID">The ID of the track to which this bit of volume data applies.</param>
		///<param name="volumeDataBuffer">The raw data containing the volume information.</param>
		///<param name="bufferSize">The number of bytes volume data.</param>
		///<returns>Integer return code specifying the result of the call.</returns>
		int sendVolumeData(sampleid_t sampleID, trackid_t trackID, char* volumeDataBuffer, int bufferSize);

		///<summary>Single entry point for all network communications. Reads the control byte and acts on it accordingly.</summary>
		///<param name="datagram">The datagram data.</param>
		void dispatchNetworkMessage(char* datagram);

	public:

		void testStart();

	};


};