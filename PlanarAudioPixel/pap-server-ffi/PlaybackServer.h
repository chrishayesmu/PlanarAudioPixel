#pragma once

#include "../pap-networking/PlaybackServer.h"
#pragma comment(lib, "../Debug/pap-networking.lib")

using namespace System;

namespace PlanarAudioPixel {

	///<summary>Main interface class to access lower level Networking::PlaybackServer.</summary>
	public ref class PlaybackServer {
	private:
		Networking::PlaybackServer* server;

	public:

		///<summary>Constructs a playback server foreign function interface class.</summary>
		PlaybackServer();

		///<summary>Starts the playback server.</summary>
		void Start();

	};

};