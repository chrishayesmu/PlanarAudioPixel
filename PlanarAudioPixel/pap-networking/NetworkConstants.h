// -----------------------------------------------------------------
// This file is intended as a helper file, which will include all of
// the other networking files which define constants. It will also
// define some constants which are not defined within other files.
// -----------------------------------------------------------------

#include "ControlByteConstants.h"

namespace Networking
{
	// The maximum number of clients who can be connected to the playback server
	// simultaneously. If the number of clients connected meets this number, then
	// any further connections should be rejected.
	unsigned int MAX_SIMULTANEOUS_CLIENTS = 30;

	// The minimum volume threshold which can be reached before the server simply
	// rounds the volume calculation down to 0. This is used to prevent clients from
	// wasting time playing samples at a volume which could not be heard.
	float MIN_VOLUME_THRESHOLD = 0.025f;
}