#ifndef _sockets
#define _sockets

#include <winsock2.h>
#include <WS2tcpip.h>
#include <io.h>

#define open _open
#define write _write
#define read _read

/*
struct sockaddr_in {
    sa_family_t    sin_family; // address family: AF_INET //
    in_port_t      sin_port;   // port in network byte order //
    struct in_addr sin_addr;   // internet address //
};

// Internet address. //
struct in_addr {
    uint32_t       s_addr;     // address in network byte order //
};

struct hostent {
    char  *h_name;            // official name of host //
    char **h_aliases;         // alias list //
    int    h_addrtype;        // host address type //
    int    h_length;          // length of address //
    char **h_addr_list;       // list of addresses //
}

struct sockaddr {
    unsigned short    sa_family;    // address family, AF_xxx
    char              sa_data[14];  // 14 bytes of protocol address
}

*/

typedef struct _socket {
	struct sockaddr_in 	ADDRESS;
	struct hostent*		HOST;
	char				HOSTNAME[512];
	int					sfd;
	
	struct sockaddr* 	CONNECTED_ADDRESS;
	socklen_t			CONNECTED_ADDRESS_LENGTH;
} _socket;

//creates a new _socket whose purpose is to act as a server socket and accept incoming connections
_socket* sockets_server_socket(unsigned short port);

//connects to a listening socket at the hostname on port port
_socket* sockets_connect_to(char* __restrict hostname, unsigned short port);

//returns connected socket fd on success and -1 on timeout or socket accept error
//AND FILLS OUT CONNECTED SOCKET INFORMATION IN s
int sockets_accept(_socket* __restrict s, int timeout);

int sockets_send_message(_socket* __restrict s, const void* __restrict data, size_t size);

int sockets_receive_message(_socket* __restrict s, void* __restrict data, size_t size);


#endif