FILES=Logger.o sf_virtual_wrapper.o PapAudioPlayer.o sockets.o Timing.o
EXTRAFLAGS=-g -DNDEBUG -DPLAYBACKCLIENT -std=c++0x -Wall
FLAGS=-lsndfile -lportaudio ${EXTRAFLAGS} 

all: ${FILES} test_client.cpp 
	g++ test_client.cpp ${FLAGS} ${FILES} -o client

Logger.o: ../pap-file-io/Logger.cpp
	g++ ${EXTRAFLAGS} ../pap-file-io/Logger.cpp -c

sf_virtual_wrapper.o: sf_virtual_wrapper.c
	g++ ${EXTRAFLAGS} sf_virtual_wrapper.c -lsndfile -c

PapAudioPlayer.o: PapAudioPlayer.cpp
	g++ ${EXTRAFLAGS} PapAudioPlayer.cpp -lportaudio -c

sockets.o: sockets.cpp
	g++ ${EXTRAFLAGS} sockets.cpp -c

Timing.o: Timing.cpp

clean:
	rm -f a.out PlaybackClient *.o  log*.txt client
