#include <stdio.h>
#include "sockets.h"
#include "NetworkStructures.h"
#include "../pap-file-io/IOStructures.h"
#include "ControlByteConstants.h"
#include "sockets.h"

#define PAP_NO_STDOUT
#include "PapAudioPlayer.hpp"
#include "../pap-file-io/Logger.h"

#include <stdio.h>      
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <string.h> 
#include <arpa/inet.h>

#include <sys/time.h>

#include "Timing.hpp"

#define AUDIO_PACKET_SIZE 1500

int main(int argc, char** argv) {

	if (argc != 2)
	{
		printf("Usage: %s <server-host-or-ip>\n", argv[0]);
		return 1;
	}

	Logger::openLogFile();

	PapAudioPlayer* audioPlayer = NULL;

	Networking::ClientGUID clientID;
	Networking::IP_Address clientLocalIP;
	
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;

    getifaddrs(&ifAddrStruct);

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa ->ifa_addr->sa_family==AF_INET) { // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
			Logger::logNotice("%s IP Address %s", ifa->ifa_name, addressBuffer); 
			if (ifa->ifa_name[0] == 'e' &&
				ifa->ifa_name[1] == 't' &&
				ifa->ifa_name[2] == 'h') {
				sscanf(addressBuffer, "%hu.%hu.%hu.%hu", &clientLocalIP.Byte1, &clientLocalIP.Byte2, &clientLocalIP.Byte3, &clientLocalIP.Byte4);
				break;
			}
        }
    }

	

	clientID = clientLocalIP.RawIP;
	
	_socket* s = sockets_connect_to(argv[1], 32746);
	
	if (!s) {
		Logger::logError("Failed to connect");
	}
	else
	{
		Logger::logNotice("Connected to remote server");
	}
	
	Networking::PacketStructures::NetworkMessage connectionPacket;
	
	int piID = clientLocalIP.Byte4 - 101;
	float posX = (float)(piID % 6);
	float posY = (float)(piID / 6);

	connectionPacket.ClientConnection.position.x = posX;
	connectionPacket.ClientConnection.position.y = posY;
	connectionPacket.ClientConnection.clientID = clientID;
	
	sockets_send_message(s->sfd, &connectionPacket, sizeof( connectionPacket ));
	
	scottgs::Timing timer;
	int totalBytesRead = 0;
	timer.start();

	int trueTotalBytes = 0;
	double trueTotalTime = 0.0;

	bool endOfSamples = false;

	while (1) {
		
		Networking::PacketStructures::NetworkMessage message;
		message.ControlByte = 0;
		char extra_data[AUDIO_PACKET_SIZE];
		int readCount;
		int expectedNumSamples;

		readCount = sockets_receive_exactly(s->sfd, &message, sizeof(message));
		if (readCount == 0) break;
		
		Logger::logNotice("Read %d bytes with control byte %d", readCount, message.ControlByte);

		switch (message.ControlByte) {
			case Networking::ControlBytes::SENDING_AUDIO:
				readCount = sockets_receive_exactly(s->sfd, extra_data, message.Extra._dataLength);
				totalBytesRead += message.Extra._dataLength;

				expectedNumSamples = message.AudioSample.fileSize / AUDIO_PACKET_SIZE;
				if (message.AudioSample.SampleID == expectedNumSamples - 1)
				{
					endOfSamples = true;
					printf("Last sample received (sample ID %d, with %d samples expected total)\n", message.AudioSample.SampleID, expectedNumSamples);
				}

//				Logger::logNotice("Received audio packet: %d (%d/%lld); %d total", message.AudioSample.SampleID, readCount, message.Extra._dataLength, totalBytesRead);
				//printf("Got sample %d\n", message.AudioSample.SampleID);
				totalBytesRead += message.Extra._dataLength;
				if (audioPlayer == NULL)
				{
					audioPlayer = new PapAudioPlayer(message.AudioSample.fileSize, AUDIO_PACKET_SIZE);
					audioPlayer->bufferAudioData(extra_data, message.Extra._dataLength);
					audioPlayer->setup();
				}
				else
				{
					audioPlayer->bufferAudioData(extra_data, message.Extra._dataLength);
				}

				//audioPlayer->setVolumeData(message.AudioSample.SampleID, 0.75f);
				
				if (timer.getTotalElapsedTime() > 1.0 && !endOfSamples)
				{
					//printf("Data rate of %lf B/s (of %d bytes, %lf seconds)\n", totalBytesRead / timer.getTotalElapsedTime(), totalBytesRead, timer.getTotalElapsedTime());

					trueTotalBytes += totalBytesRead;
					trueTotalTime += timer.getTotalElapsedTime();
					totalBytesRead = 0;
					timer.reset();
					timer.start();
				}

				if (audioPlayer->endOfBuffer() && !audioPlayer->isAudioPlaying())
				{
					goto done_playing;
				}

			break;
			case Networking::ControlBytes::SENDING_VOLUME:
				if (audioPlayer != NULL)
				{
					audioPlayer->setVolumeData(message.VolumeSample.SampleID, message.VolumeSample.volume);
				}
			break;
			case Networking::ControlBytes::BEGIN_PLAYBACK:
				Logger::logNotice("Received BEGIN_PLAYBACK message");
				timer.stop();
				struct timeval tv;
				if (audioPlayer != NULL)
				{
					gettimeofday(&tv, NULL);
					long long timeInMs = 1000L * ((long long) tv.tv_sec) + tv.tv_usec / 1000L;
					printf("Should be playing at time %llu; current time is %llu\n", message.TransportControl.timeOffset, timeInMs);

					// Block until it's time to play
					while (timeInMs < message.TransportControl.timeOffset)
					{
						gettimeofday(&tv, NULL);
						timeInMs = 1000L * ((long long) tv.tv_sec) + tv.tv_usec / 1000L;
					//	printf("time: %lu\n", timeInMs);
					}

					audioPlayer->play();
				}
				timer.start();
			break;
			case Networking::ControlBytes::PAUSE_PLAYBACK:
				Logger::logNotice("Received PAUSE_PLAYBACK message");
				if (audioPlayer != NULL)
				{
					audioPlayer->pause();
				}
			break;
			case Networking::ControlBytes::STOP_PLAYBACK:
				Logger::logNotice("Received STOP_PLAYBACK message");
				// TODO: stop audio player
				delete audioPlayer;
				audioPlayer = NULL;
				goto done_playing;
			break;
		}
		
	}
	
	done_playing:
	printf("Recorded a total of %d bytes during the song\n", trueTotalBytes);
	printf("Average data rate: %lf B/s\n", trueTotalBytes / trueTotalTime);	

	printf("Disconnected.\n");
	
}
