#pragma once

#include "../pap-networking/PlaybackServer.h"
#pragma comment(lib, "../Debug/pap-networking.lib")

using namespace System;

namespace PlanarAudioPixel {

	///<summary>A set of PlaybackServer error codes for the return values of certain functions.</summary>
	public enum PlaybackServerErrorCodes {
		PlaybackServer_OK = 0,
		PlaybackServer_INVALID = 1,
		PlaybackServer_FILE = 2,
		PlaybackServer_POINTER = 3
	};
	typedef PlaybackServerErrorCodes PlaybackServerErrorCode;

	//Callback delegate for adding a track
	public delegate void AddTrackCallback(int audioFileErrorCode, int positionFileErrorCode);
		
	///<summary>Provides a chaining callback to link the unmanaged world back to the managed world.</summary>
	static void trackCallback(int audioCode, int positionCode, uint64_t token);

	///<summary>Main interface class to access lower level Networking::PlaybackServer.</summary>
	public ref class PlaybackServer {
	private:
		Networking::PlaybackServer* server;

		static uint64_t AddTrackRequestID = 0;

	public:
		
		static System::Collections::Generic::Dictionary<uint64_t, AddTrackCallback^>^ _TrackCallback = gcnew System::Collections::Generic::Dictionary<uint64_t, AddTrackCallback^>();

		///<summary>Constructs a playback server foreign function interface class.</summary>
		PlaybackServer();
		
		///<summary>Attempts to start the PlaybackServer.</summary>
		///<returns>A PlaybackServerErrorCode indicating the result of this call.</returns>
		void ServerStart();
		
		///<summary>Attempts to start playback.</summary>
		///<returns>A PlaybackErrorCode indicating the result of this call.</returns>
		void Play();
		
		///<summary>Attempts to pause playback.</summary>
		///<returns>A PlaybackErrorCode indicating the result of this call.</returns>
		void Pause();

		///<summary>Attempts to stop playback.</summary>
		///<returns>A PlaybackErrorCode indicating the result of this call.</returns>
		void Stop();
		
		///<summary>Queues up a request to asynchronously add a track to the server's playlist.</summary>
		///<param name="audioFilename">The name of the audio file to add.</param>
		///<param name="positionFilename">The name of the corresponding position data file.</param>
		///<returns>A PlaybackServerErrorCode indicating the result of this call.</returns>
		void AddTrack(System::String^ audioFilename, System::String^ positionFilename);

		///<summary>Queues up a request to asynchronously add a track to the server's playlist.</summary>
		///<param name="audioFilename">The name of the audio file to add.</param>
		///<param name="positionFilename">The name of the corresponding position data file.</param>
		///<param name="callback">The delegate callback to fire when this request finishes processing.</param>
		///<returns>A PlaybackServerErrorCode indicating the result of this call.</returns>
		void AddTrack(System::String^ audioFilename, System::String^ positionFilename, AddTrackCallback^ callback);

	};
	
};