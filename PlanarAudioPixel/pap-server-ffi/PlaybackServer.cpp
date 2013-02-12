
#include "PlaybackServer.h"
#include <vcclr.h>

namespace PlanarAudioPixel {
	
	///<summary>Provides a chaining callback to link the unmanaged world back to the managed world.</summary>
	static void trackCallback(int audioCode, int positionCode, uint64_t token) {
		PlaybackServer::_TrackCallback[token]->Invoke(audioCode, positionCode);
		PlaybackServer::_TrackCallback->Remove(token);
	}

	//Event callback chaining functions

	static void clientConnectedChain(Networking::Client c) {
		Client clientChain;
		clientChain.ClientID.BroadcastIP.RawIP = c.ClientID.BroadcastIP.RawIP;
		clientChain.ClientID.LocalIP.RawIP = c.ClientID.LocalIP.RawIP;
		clientChain.LastCheckInTime = c.LastCheckInTime;
		clientChain.Offset.x = c.Offset.x;
		clientChain.Offset.y = c.Offset.y;

		for (int i = 0; i < PlaybackServer::_ClientConnectedCallbacks->Count; ++i)
			PlaybackServer::_ClientConnectedCallbacks[i]->Invoke(clientChain);
	}

	static void clientDisconnectedChain(Networking::Client c) {
		Client clientChain;
		clientChain.ClientID.BroadcastIP.RawIP = c.ClientID.BroadcastIP.RawIP;
		clientChain.ClientID.LocalIP.RawIP = c.ClientID.LocalIP.RawIP;
		clientChain.LastCheckInTime = c.LastCheckInTime;
		clientChain.Offset.x = c.Offset.x;
		clientChain.Offset.y = c.Offset.y;

		for (int i = 0; i < PlaybackServer::_ClientConnectedCallbacks->Count; ++i)
			PlaybackServer::_ClientDisconnectedCallbacks[i]->Invoke(clientChain);
	}

	static void clientCheckInChain(Networking::Client c) {
		Client clientChain;
		clientChain.ClientID.BroadcastIP.RawIP = c.ClientID.BroadcastIP.RawIP;
		clientChain.ClientID.LocalIP.RawIP = c.ClientID.LocalIP.RawIP;
		clientChain.LastCheckInTime = c.LastCheckInTime;
		clientChain.Offset.x = c.Offset.x;
		clientChain.Offset.y = c.Offset.y;

		for (int i = 0; i < PlaybackServer::_ClientConnectedCallbacks->Count; ++i)
			PlaybackServer::_ClientCheckInCallbacks[i]->Invoke(clientChain);
	}


	///<summary>Constructs a playback server foreign function interface class.</summary>
	PlaybackServer::PlaybackServer(){

		Networking::PlaybackServer* server;
		if (SOCKETFAILED(Networking::PlaybackServer::Create(&server))){
			throw gcnew Exception(L"Could not create the playback server.");
		}

		server->OnClientConnected(&clientConnectedChain);
		server->OnClientDisconnected(&clientDisconnectedChain);
		server->OnClientCheckIn(&clientCheckInChain);

		this->server = server;

	}
	
	///<summary>Attempts to start the PlaybackServer.</summary>
	///<returns>A PlaybackServerErrorCode indicating the result of this call.</returns>
	void PlaybackServer::ServerStart(){
		Networking::PlaybackServerErrorCode result = this->server->ServerStart();
		if (result != Networking::PlaybackServerErrorCodes::PlaybackServer_OK) {
			throw gcnew Exception(L"Could not start the playback server.");
		}
	}

	
	///<summary>Attempts to start playback.</summary>
	///<returns>A PlaybackErrorCode indicating the result of this call.</returns>
	void PlaybackServer::Play() {
		Networking::PlaybackServerErrorCode result = this->server->Play();
		if (result != Networking::PlaybackServerErrorCodes::PlaybackServer_OK) {
			throw gcnew Exception(L"The server has not been started or is in an invalid state.");
		}
	}

	///<summary>Attempts to pause playback.</summary>
	///<returns>A PlaybackErrorCode indicating the result of this call.</returns>
	void PlaybackServer::Pause() {
		Networking::PlaybackServerErrorCode result = this->server->Pause();
		if (result != Networking::PlaybackServerErrorCodes::PlaybackServer_OK) {
			throw gcnew Exception(L"The server has not been started or is in an invalid state.");
		}
	}

	///<summary>Attempts to stop playback.</summary>
	///<returns>A PlaybackErrorCode indicating the result of this call.</returns>
	void PlaybackServer::Stop() {
		Networking::PlaybackServerErrorCode result = this->server->Stop();
		if (result != Networking::PlaybackServerErrorCodes::PlaybackServer_OK) {
			throw gcnew Exception(L"The server has not been started or is in an invalid state.");
		}
	}

	///<summary>Queues up a request to asynchronously add a track to the server's playlist.</summary>
	///<param name="audioFilename">The name of the audio file to add.</param>
	///<param name="positionFilename">The name of the corresponding position data file.</param>
	///<returns>A PlaybackServerErrorCode indicating the result of this call.</returns>
	void PlaybackServer::AddTrack(System::String^ audioFilename, System::String^ positionFilename) {

		pin_ptr<const wchar_t> _audioFilename = PtrToStringChars(audioFilename);

		size_t size = (audioFilename->Length + 1)<<1;
		char* __afn = (char*)malloc(size);
		wcstombs_s(&size, __afn, size, _audioFilename, size);

		pin_ptr<const wchar_t> _positionFilename = PtrToStringChars(positionFilename);

		size_t sizeo = (positionFilename->Length + 1)<<1;
		char* __pfn = (char*)malloc(sizeo);
		wcstombs_s(&sizeo, __pfn, sizeo, _positionFilename, sizeo);

		Networking::PlaybackServerErrorCode result = this->server->AddTrack(__afn, __pfn);
		if (result != Networking::PlaybackServerErrorCodes::PlaybackServer_OK) {
			throw gcnew Exception(L"The server has not been started or is in an invalid state.");
		}
	}
	
	///<summary>Queues up a request to asynchronously add a track to the server's playlist.</summary>
	///<param name="audioFilename">The name of the audio file to add.</param>
	///<param name="positionFilename">The name of the corresponding position data file.</param>
	///<param name="callback">The delegate callback to fire when this request finishes processing.</param>
	///<returns>A PlaybackServerErrorCode indicating the result of this call.</returns>
	void PlaybackServer::AddTrack(System::String^ audioFilename, System::String^ positionFilename, AddTrackCallback^ callback) {
		
		pin_ptr<const wchar_t> _audioFilename = PtrToStringChars(audioFilename);

		size_t size = (audioFilename->Length + 1)<<1;
		char* __afn = (char*)malloc(size);
		wcstombs_s(&size, __afn, size, _audioFilename, size);

		pin_ptr<const wchar_t> _positionFilename = PtrToStringChars(positionFilename);

		size_t sizeo = (positionFilename->Length + 1)<<1;
		char* __pfn = (char*)malloc(sizeo);
		wcstombs_s(&sizeo, __pfn, sizeo, _positionFilename, sizeo);
		
		PlaybackServer::_TrackCallback->Add(this->AddTrackRequestID, callback);
		Networking::PlaybackServerErrorCode result = this->server->AddTrack(__afn, __pfn, &PlanarAudioPixel::trackCallback, this->AddTrackRequestID);
		++this->AddTrackRequestID;

		if (result != Networking::PlaybackServerErrorCodes::PlaybackServer_OK) {
			throw gcnew Exception(L"The server has not been started or is in an invalid state.");
		}

	}

			
	///<summary>Subscribes the caller to the ClientConnected event. ClientConnected is raised when a new client appears on the network.</summary>
	///<param name="callback">A pointer to the function to call when the event is raised.</param>
	void PlaybackServer::OnClientConnected(ClientConnectedCallback^ callback) {
		PlaybackServer::_ClientConnectedCallbacks->Add(callback);
	}

	///<summary>Subscribes the caller to the ClientDisconnected event. ClientDisconnected is raised when a client disconnects from the network.</summary>
	///<param name="callback">A pointer to the function to call when the event is raised.</param>
	void PlaybackServer::OnClientDisconnected(ClientDisconnectedCallback^ callback) {
		PlaybackServer::_ClientDisconnectedCallbacks->Add(callback);
	}

	///<summary>Subscribes the caller to the ClientCheckIn event. ClientCheckIn is raised when a client checks in to the network.</summary>
	///<param name="callback">A pointer to the function to call when the event is raised.</param>
	void PlaybackServer::OnClientCheckIn(ClientCheckInCallback^ callback) {
		PlaybackServer::_ClientCheckInCallbacks->Add(callback);
	}


};