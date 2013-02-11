
#include "PlaybackServer.h"
#include <vcclr.h>

namespace PlanarAudioPixel {
	
	///<summary>Provides a chaining callback to link the unmanaged world back to the managed world.</summary>
	static void trackCallback(int audioCode, int positionCode, uint64_t token) {
		PlaybackServer::_TrackCallback[token]->Invoke(audioCode, positionCode);
		PlaybackServer::_TrackCallback->Remove(token);
	}

	///<summary>Constructs a playback server foreign function interface class.</summary>
	PlaybackServer::PlaybackServer(){



		Networking::PlaybackServer* server;
		if (SOCKETFAILED(Networking::PlaybackServer::Create(&server))){
			throw gcnew Exception(L"Could not create the playback server.");
		}

		this->server = server;

	}
	
	///<summary>Attempts to start the PlaybackServer.</summary>
	///<returns>A PlaybackServerErrorCode indicating the result of this call.</returns>
	void PlaybackServer::Start(){
		Networking::PlaybackServerErrorCode result = this->server->Start();
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

};