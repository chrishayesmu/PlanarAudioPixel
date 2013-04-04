#pragma once

#ifndef RASPBERRY_PI

#include <winsock2.h>
#include <WS2tcpip.h>

#else

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
typedef int SOCKET;
#define INVALID_SOCKET -1
#define SOCKET_ERROR   -1
#define LPCTSTR const char*
 
#endif

#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")

#undef SendMessage

namespace Networking {

	// Macros for error checking
#define SOCKETFAILED(x) FAILED(x)
#define SOCKETSUCEEEDED(x) SUCCEEDED(x)
#define SOCKETTIMEOUT(x) (x == SocketErrorCode::SocketError_TIMEOUT)

	// Error codes for socket functions
	enum SocketErrorCode {
		SocketError_OK = 0,
		SocketError_POINTER = -4,
		SocketError_INVALIDSOCKET = -2,
		SocketError_IPNOTBOUND = -3,
		SocketError_SOCKETERROR = SOCKET_ERROR,
		SocketError_PORTNOTBOUND = -5,
		SocketError_TIMEOUT = -6
	};

	// Socket types for creation
	enum SocketType : int {
		TCP = 1,
		UDP = 2,
		RAW = 3,
		RDM = 4,
		SEQPACKET = 5
	};

	// Address Family types for creation
	enum AddressFamily : int {
		//The address family is unspecified.
		Unspecified = AF_UNSPEC,
		//The Internet Protocol version 4 (IPv4) address family.
		IPv4 = AF_INET,
		//The NetBIOS address family. This address family is only supported if a Windows Sockets provider for NetBIOS is installed.
		#ifndef RASPBERRY_PI
		NetBIOS = AF_NETBIOS,
		#else
		NetBIOS = AF_INET6,
		#endif
		//The Internet Protocol version 6 (IPv6) address family.
		IPv6 = AF_INET6,
		//The Infrared Data Association (IrDA) address family. This address family is only supported if the computer has an infrared port and driver installed.
		InfraredDataAssociation = AF_IRDA,
		//The Bluetooth address family. This address family is only supported if a Bluetooth adapter is installed on Windows Server 2003 or later.
		#ifndef RASPBERRY_PI
		Bluetooth = AF_BTH
		#else
		Bluetooth = 32
		#endif
	};

	#ifdef UNICODE
	///<summary>Returns the string representation of the error code <paramref name="s" />.</summary>
	///<param name="s">The error code returned by any of the Socket class functions.</param>
	///<returns>A lexical representation of the error code as a wide character string.</returns>
	LPCWSTR SocketErrorToString(int s);
	#else
	///<summary>Returns the string representation of the error code <paramref name="s" />.</summary>
	///<param name="s">The error code returned by any of the Socket class functions.</param>
	///<returns>A lexical representation of the error code as a character string.</returns>
	LPCTSTR SocketErrorToString(int s);
	#endif

	///<summary>A class for creating and using winsock socket connections.</summary>
	class Socket {
	private:

		// Address info for this connection.
		struct addrinfo addressInfo;

		// Preconditions for whether or not the receive and send binds have been called.
		bool boundRecvPort,
			 boundSendIP;

		// Separate sockets for sending and receiving and their respective addresses.
		SOCKET recvSocket, sendSocket;
		sockaddr_in recvAddress, sendAddress;

		// A private constructor for the class. Creation should be done through the Create() static function
		// which will return an error code if the socket could not be created with the given parameters.
		Socket();

	public:

		#ifndef RASPBERRY_PI
		// Winsock data
		WSADATA wsaData;
		#endif

		// Internal socket type (TCP, UDP, etc..)
		SocketType socketType;

		///<summary>Creates an unspecified address family socket with the given socket type.</summary>
		///<param name="s">A reference to the Socket* to fill with a new Socket class.</param>
		///<param name="t">The socket type for this socket from the SocketType enum. (TCP, UDP, etc..).</param>
		///<returns>An integer error code. Errors can be printed using the SocketErrorToString() function.</returns>
		static int Create(Socket** s, SocketType t);

		///<summary>Creates socket with the given socket type.</summary>
		///<param name="s">A reference to the Socket* to fill with a new Socket class.</param>
		///<param name="t">The socket type for this socket from the SocketType enum. (TCP, UDP, etc..).</param>
		///<param name="AddressFamily">The specific address family to which this socket applies.</param>
		///<returns>An integer error code. Errors can be printed using the SocketErrorToString() function.</returns>
		static int Create(Socket** s, SocketType t, AddressFamily af);

		///<summary>Bind a receiving port number for connectionless socket types.</summary>
		///<param name="port">The number of the port to bind.</param>
		///<returns>An integer error code. Errors can be printed using the SocketErrorToString() function.</returns>
		int BindRecvPort(unsigned short port);

		///<summary>Bind a sending port number and IP address for sending data.</summary>
		///<param name="port">The number of the port to bind.</param>
		///<param name="IP">The string representation of the IP address to bind.</param>
		///<returns>An integer error code. Errors can be printed using the SocketErrorToString() function.</returns>
		void BindSendIP(unsigned short port, const char* IP);

		///<summary>Helper function to set this socket up as a receiving UDP socket.</summary>
		///<param name="port">The port to receive data from.</param>
		///<returns>An integer error code. Errors can be printed using the SocketErrorToString() function.</returns>
		int PrepareUDPReceive(unsigned short port);

		///<summary>Helper function to set this socket up as a sending UDP socket.</summary>
		///<param name="port">The port to send data on.</param>
		///<param name="IP">The string representation of the IP address to send to.</param>
		///<returns>An integer error code. Errors can be printed using the SocketErrorToString() function.</returns>
		void PrepareUDPSend(unsigned short port, const char* IP);

		///<summary>Sends a message on this socket.</summary>
		///<param name="message">A pointer to the data to send.</param>
		///<param name="messageLength">The length of the data to set.</param>
		///<returns>An integer error code. Errors can be printed using the SocketErrorToString() function.</returns>
		int SendMessage(const char* message, int messageLength);
	
		///<summary>Blocking call that receives a message on this socket.</summary>
		///<param name="buffer">The buffer in which to store the received message.</param>
		///<param name="buffersize">The maximum length of the buffer.</param>
		///<param name="sender">A reference to the sockadd_in struct to fill with the information regarding the sending.</param>
		///<param name="senderSize">A reference to an integer to fill with the byte length of <paramref name="sender" />.</param>
		///<returns>An integer error code. Errors can be printed using the SocketErrorToString() function.</returns>
		int ReceiveMessage(char* buffer, int buffersize, sockaddr_in* sender, int* senderSize);
	
		///<summary>Blocking call that receives a message on this socket.</summary>
		///<param name="buffer">The buffer in which to store the received message.</param>
		///<param name="buffersize">The maximum length of the buffer.<param>
		///<returns>An integer error code. Errors can be printed using the SocketErrorToString() function.</returns>
		int ReceiveMessage(char* buffer, int buffersize);
	
		///<summary>Blocking call that receives a message on this socket.</summary>
		///<param name="buffer">The buffer in which to store the received message.</param>
		///<param name="buffersize">The maximum length of the buffer.</param>
		///<param name="msCount">The number of milliseconds to wait for the socket to have data available.</param>
		///<param name="sender">A reference to the sockadd_in struct to fill with the information regarding the sending.</param>
		///<param name="senderSize">A reference to an integer to fill with the byte length of <paramref name="sender" />.</param>
		///<returns>An integer error code. Errors can be printed using the SocketErrorToString() function. Timeout can be checked with the SOCKETTIMEOUT(x) macro.</returns>
		int TryReceiveMessage(char* buffer, int buffersize, time_t msCount, sockaddr_in* sender, int* senderSize);
	
		///<summary>Attempts to recieve a message for the specified number of milliseconds.</summary>
		///<param name="buffer">The buffer in which to store the received message.</param>
		///<param name="buffersize">The maximum length of the buffer.</param>
		///<param name="msCount">The number of milliseconds to wait for the socket to have data available.</param>
		///<returns>An integer error code. Errors can be printed using the SocketErrorToString() function. Timeout can be checked with the SOCKETTIMEOUT(x) macro.</returns>
		int TryReceiveMessage(char* buffer, int buffersize, time_t msCount);

	};

}