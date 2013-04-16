
#include "sockets.h"

//creates a new _socket whose purpose is to act as a server socket and accept incoming connections
_socket* sockets_server_socket(unsigned short port){
	_socket* s = (_socket*)calloc(1, sizeof(_socket));
	
	//obtain host name
	if (gethostname(s->HOSTNAME, 512)==-1) return NULL;
	//get host info
	if (!(s->HOST = gethostbyname(s->HOSTNAME))) return NULL;
	
	//initiate the ADDRESS using the HOST information and given port
	s->ADDRESS.sin_family = s->HOST->h_addrtype;
	s->ADDRESS.sin_port = htons(port);
	
	//create the socket
	if ((s->sfd = socket(AF_INET, SOCK_STREAM, 0))==-1) return NULL;
	
	//bind this socket to prepare for listening
	if (bind(s->sfd, (struct sockaddr*)s, sizeof(struct sockaddr_in)) ==-1) return NULL;
	
	//begin listening on this socket
	if (listen(s->sfd, 128)==-1) return NULL;
	
	return s;
}

//connects to a listening socket at the hostname on port port
_socket* sockets_connect_to(char* hostname, unsigned short port){
	_socket* s = (_socket*)calloc(1, sizeof(_socket));
	
	//get host info
	if (!(s->HOST = gethostbyname(hostname))) return NULL;
	
	//copy the host's first address information into our socket address
	memcpy((char*)(&s->ADDRESS.sin_addr),
				s->HOST->h_addr_list[0],
					s->HOST->h_length);
	
	//initiate the ADDRESS using the HOST information and given port
	s->ADDRESS.sin_family= s->HOST->h_addrtype;
	s->ADDRESS.sin_port = htons(port);
	
	//create a TCP/IP socket using the host's address type
	if ((s->sfd=socket(s->HOST->h_addrtype,SOCK_STREAM,0))==-1) return NULL;
	
	//attempt to connect this socket to the host
	if (connect(s->sfd,(struct sockaddr*)s,sizeof(struct sockaddr_in))==-1) return NULL;
	
	return s;
}

//returns connected socket fd on success and -1 on timeout or socket accept error
//AND FILLS OUT CONNECTED SOCKET INFORMATION IN s
int sockets_accept(_socket* s, int timeout){
	struct pollfd fds[1] = {{s->sfd, POLLIN, 0}};
	if (!WSAPoll(fds, 1, timeout)) return -1;
	s->CONNECTED_ADDRESS_LENGTH = sizeof(struct sockaddr_in);
	return accept(s->sfd, s->CONNECTED_ADDRESS, &s->CONNECTED_ADDRESS_LENGTH);
}

int sockets_send_message(int sfd, const void* __restrict data, size_t size) {
	return sendto(sfd, (const char*)data, size, 0, NULL, NULL);
}

int sockets_receive_message(int sfd, void* __restrict data, size_t size) {
	return recvfrom(sfd, (char*)data, size, 0, NULL, NULL);
}