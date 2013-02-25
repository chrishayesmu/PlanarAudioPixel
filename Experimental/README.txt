These files are not to be added to the project itself, but rather to serve as a starting point for audio parsing.

From my research I found out the following:
1) We can parse .wav files to chunks of time size. This is cool because if we stick to .wavs by either converting files in program, or by forcing users to use wavs, we can make the audio splitting pretty nice.
2) Parsing .wav files by data size will be trickier. We can A) send a packet with the wav header's information, followed by packets of raw data or B) parse full wav files to a close approximation of filesize. I think the latter will be very hard, and probably not useful. Which means we may have to revise our packet model.
3) Doing audio stuff on windows is easy. Doing it on Linux is unpleasant. We need to find better tools.

Questions
1) What are some better ways of dealing with audio on linux in c++?
2) How are we parsing audio files?
3) how are we transfering audio files through udp? ( raw pcm data, or self contained files )
4) will we base on file play time length, or packetsize?
5) What types of audio files are we trying to play?

Resources in no particular order:
http://www.volkerschatz.com/noise/sndutils.html

http://www.daniweb.com/software-development/cpp/threads/367488/reading-in-wave-file

http://www.alsa-project.org/alsa-doc/alsa-lib/examples.html

