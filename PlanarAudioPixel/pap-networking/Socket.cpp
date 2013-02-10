#include "Socket.h"

namespace Networking {

	Socket::Socket(){
		//Clear bound flags on this socket.
		this->boundRecvPort = false;
		this->boundSendIP = false;
	}

	int Socket::Create(Socket** s, SocketType t){
		//Call the general creation method with AddressFamily_Unspecified
		return Socket::Create(s, t, AddressFamily::Unspecified);
	}
	int Socket::Create(Socket** s, SocketType t, AddressFamily af){
		if (!s) return SocketError_POINTER;

		int hr = SocketError_OK;
		Socket* sc = new Socket();

		// Initialize Winsock; Has no affect if Winsock is already initialized.
		hr = WSAStartup(MAKEWORD(2,2), &sc->wsaData);
		if (SOCKETFAILED(hr)) 
			goto cleanup;

		// Set up the socket
		sc->socketType = t;

		// Set up the address info for this socket
		ZeroMemory(&sc->addressInfo, sizeof(sc->addressInfo));
		sc->addressInfo.ai_family = af;

		// Set up sending and receiving address information
		sc->recvSocket = INVALID_SOCKET;
		sc->sendSocket = INVALID_SOCKET;
		switch (t){
		case TCP:
			sc->recvSocket = socket(af, SOCK_STREAM, IPPROTO_TCP);
			sc->sendSocket = socket(af, SOCK_STREAM, IPPROTO_TCP);
			sc->addressInfo.ai_socktype = SOCK_STREAM;
			sc->addressInfo.ai_protocol = IPPROTO_TCP;
			break;
		case UDP:
			sc->recvSocket = socket(af, SOCK_DGRAM, IPPROTO_UDP);
			sc->sendSocket = socket(af, SOCK_DGRAM, IPPROTO_UDP);
			sc->addressInfo.ai_socktype = SOCK_DGRAM;
			sc->addressInfo.ai_protocol = IPPROTO_UDP;
			break;
		}

		// If the set up failed, return false
		if (sc->recvSocket == INVALID_SOCKET || sc->sendSocket == INVALID_SOCKET) 
			hr = SocketError_INVALIDSOCKET;

	cleanup:
		if (SOCKETFAILED(hr)){
			delete sc;
			*s = NULL;
		} else 
			*s = sc;

		return hr;
	}

	int Socket::BindRecvPort(unsigned short port){

		// If the socket is already bound on a port, reset the socket.
		if (this->boundRecvPort){
			this->boundRecvPort = false;
			closesocket(this->recvSocket);

			this->recvSocket = INVALID_SOCKET;
			this->recvSocket = socket(
				this->addressInfo.ai_family,
				this->addressInfo.ai_socktype,
				this->addressInfo.ai_protocol
				);

			if (this->recvSocket == INVALID_SOCKET) 
				return SocketError_INVALIDSOCKET;
		}

		// Bind this socket to the port.
		this->recvAddress.sin_family		= this->addressInfo.ai_family;
		this->recvAddress.sin_port			= htons(port);
		this->recvAddress.sin_addr.s_addr	= htonl(INADDR_ANY);

		int r = bind(this->recvSocket, (SOCKADDR*)(&this->recvAddress), sizeof(this->recvAddress));

		if (!r)
			boundRecvPort = true;
		return r;
	}

	void Socket::BindSendIP(unsigned short port, const char* IP){
		//Set up sending information. There isn't any real "binding" that needs to be done for sending.
		this->sendAddress.sin_family = this->addressInfo.ai_family;
		this->sendAddress.sin_port = htons(port);
		this->sendAddress.sin_addr.s_addr = inet_addr(IP);
		this->boundSendIP = true;
	}

	int Socket::PrepareUDPReceive(unsigned short port){
		int r = 0;

		// UDP receiving requires binding the receive port and sending IP
		if (SOCKETFAILED(r = this->BindRecvPort(port))) 
			return r;

		return SocketError_OK;
	}

	void Socket::PrepareUDPSend(unsigned short port, const char* IP){
		// UPD sending only requires setting up the IP information
		this->BindSendIP(port, IP);
	}

	int Socket::SendMessage(const char* message, int messageLength){
		if (!this->boundSendIP) 
			return SocketError_IPNOTBOUND;
		int r = sendto(this->sendSocket,
			message, messageLength, 0, (SOCKADDR*)(&this->sendAddress), sizeof(this->sendAddress));
		return r;
	}

	int Socket::ReceiveMessage(char* buffer, int buffersize, sockaddr_in* sender, int* senderSize){
		if (!this->boundRecvPort) return SocketError_PORTNOTBOUND;
		if (!buffer) return SocketError_POINTER;
		int r = recvfrom(this->recvSocket,
			buffer, buffersize, 0, (SOCKADDR*)sender, senderSize);
		return r;
	}
	int Socket::ReceiveMessage(char* buffer, int buffersize){
		return this->ReceiveMessage(buffer, buffersize, NULL, NULL);
	}

	
	///<summary>Blocking call that receives a message on this socket.</summary>
	///<param name="buffer">The buffer in which to store the received message.</param>
	///<param name="buffersize">The maximum length of the buffer.</param>
	///<param name="sender">A reference to the sockadd_in struct to fill with the information regarding the sending.</param>
	///<param name="senderSize">A reference to an integer to fill with the byte length of <paramref name="sender" />.</param>
	///<returns>An integer error code. Errors can be printed using the SocketErrorToString() function.</returns>
	int Socket::TryReceiveMessage(char* buffer, int buffersize, time_t msCount, sockaddr_in* sender, int* senderSize){
		struct timeval timeOut;
		timeOut.tv_sec = (long)(msCount / 1000);
		timeOut.tv_usec = (long)((msCount - (timeOut.tv_sec * 1000)) * 1000);

		fd_set fds;
		fds.fd_count = 1;
		fds.fd_array[0] = recvSocket;
		
		int s = select(1, &fds, NULL, NULL, &timeOut);

		if (s > 0)  return this->ReceiveMessage(buffer, buffersize, sender, senderSize);
		if (s == 0) return Networking::SocketError_TIMEOUT;
		return s;
	}
	
	///<summary>Blocking call that receives a message on this socket.</summary>
	///<param name="buffer">The buffer in which to store the received message.</param>
	///<param name="buffersize">The maximum length of the buffer.<param>
	///<returns>An integer error code. Errors can be printed using the SocketErrorToString() function.</returns>
	int Socket::TryReceiveMessage(char* buffer, int buffersize, time_t msCount){
		return this->TryReceiveMessage(buffer, buffersize, msCount, NULL, NULL);
	}

}