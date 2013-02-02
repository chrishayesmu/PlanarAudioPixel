#include "Socket.h"

namespace Networking {

	Socket::Socket(){
		//Clear bound flags on this socket.
		this->boundRecvPort = false;
		this->boundSendIP = false;
	}

	int Socket::Create(Socket** s, SocketType t){
		//Call the general creation method with AddressFamily_Unspecified
		return Socket::Create(s, t, AF_UNSPEC);
	}
	int Socket::Create(Socket** s, SocketType t, int AddressFamily){
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
		sc->addressInfo.ai_family = AddressFamily;

		// Set up sending and receiving address information
		sc->recvSocket = INVALID_SOCKET;
		sc->sendSocket = INVALID_SOCKET;
		switch (t){
		case TCP:
			sc->recvSocket = socket(AddressFamily, SOCK_STREAM, IPPROTO_TCP);
			sc->sendSocket = socket(AddressFamily, SOCK_STREAM, IPPROTO_TCP);
			sc->addressInfo.ai_socktype = SOCK_STREAM;
			sc->addressInfo.ai_protocol = IPPROTO_TCP;
			break;
		case UDP:
			sc->recvSocket = socket(AddressFamily, SOCK_DGRAM, IPPROTO_UDP);
			sc->sendSocket = socket(AddressFamily, SOCK_DGRAM, IPPROTO_UDP);
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

	int Socket::PrepareUDPReceive(unsigned short port, const char* sendIP){
		int r = 0;

		// UDP receiving requires binding the receive port and sending IP
		if (SOCKETFAILED(r = this->BindRecvPort(port))) 
			return r;
		this->BindSendIP(port, sendIP);

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

}