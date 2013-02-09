
#include "PlaybackServer.h"

namespace PlanarAudioPixel {

	///<summary>Constructs a playback server foreign function interface class.</summary>
	PlaybackServer::PlaybackServer(){

		Networking::PlaybackServer* server;
		if (SOCKETFAILED(Networking::PlaybackServer::Create(&server))){
			throw gcnew Exception(L"Could not create the playback server.");
		}

		this->server = server;

	}

	///<summary>Starts the playback server.</summary>
	void PlaybackServer::Start(){
		if (this->server->Start() != Networking::PlaybackServerStates::PlaybackServer_RUNNING) {
			throw gcnew Exception(L"Could not start the playback server.");
		}
	}

};