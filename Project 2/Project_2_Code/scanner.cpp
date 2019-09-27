#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sstream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <vector>


// skel.ru.is port number: 130.208.243.61
// Ports: 4000 - 4100

// Initialize the address
struct sockaddr_in initAddress(char* ipAddress); 
// Initialize the client socket
const int initSocket();
// Scan every port in the range provided and return a vector of all the addresses that send a response
std::vector<int> findOpenPorts(sockaddr_in& address, int& clientSocket, char* argv[]); 
// Print out the open ports found in findOpenPorts
void printOpenPorts(std::vector<int> openPortMessages);
// Solve the puzzles from the open ports
std::vector<int> findHiddenPorts(sockaddr_in& address, int& clientSocket, std::vector<std::string> openPorts);


int main(int argc, char* argv[]) {
	char* ipAddress = argv[1];

	struct sockaddr_in address = initAddress(ipAddress);
	int clientSocket = initSocket();

	std::vector<int> openPorts = findOpenPorts(address, clientSocket, argv);

	printOpenPorts(openPorts);

	//std::vector<int> hiddenPorts = findHiddenPorts(address, clientSocket, openPortMessages);
}

struct sockaddr_in initAddress(char* ipAddress) {
	struct sockaddr_in address;
	address.sin_family = AF_INET;

	// Convert the IP address into binary form
	inet_pton(AF_INET, ipAddress, &address.sin_addr);

	return address;
}

const int initSocket() {
	// Initialize a socket with which to connect to the server
	int clientSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	struct timeval timeout;

	// Specify the amount of time the socket should wait for
	// a response from each port before moving on to the next one
	timeout.tv_sec = 0.9;
	timeout.tv_usec = 1;

	// Apply the timeout to the socket
	setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

	return clientSocket;
}

std::vector<int> findOpenPorts(sockaddr_in& address, int& clientSocket, char* argv[]) {
	std::vector<int> openPorts;

	int lowPort = atoi(argv[2]);
	int highPort = atoi(argv[3]);

	for(int i = lowPort; i < highPort + 1; i++) {
		// Assign the current port number to the address
        address.sin_port = htons(i);
        
        if(sendto(clientSocket, "Tell me your secrets", sizeof("Tell me your secrets") + 1, 0, (struct sockaddr*) &address, sizeof(address)) > 0) {        
			int addressLength = sizeof(address);
			char buffer[10000];

			// Removes the previous output from the buffer
	    	memset(buffer, 0, sizeof(buffer));

	    	// Receive the output from the server and put it into the buffer
	    	recvfrom(clientSocket, buffer, 10000, 0, (struct sockaddr*) &address, (socklen_t*) &addressLength);

	    	// Convert the buffer to a string
	    	std::string bufferString = std::string(buffer);

	    	// Check if the buffer is just whitespace. If so, that means
	    	// the client didn't recieve a response
	    	if(bufferString.find_first_not_of(' ') != bufferString.npos) {
	    		// If client recieved a response, add the open port to the vector
	    		openPorts.push_back(i);
	    	} 
        }
   	}

   	return openPorts;
}

void printOpenPorts(std::vector<int> openPorts) {
	for(std::vector<int>::iterator it = openPorts.begin(); it != openPorts.end(); it++) {
		std::cout << *it << std::endl;
	}
}

std::vector<int> findHiddenPorts(sockaddr_in& address, int& clientSocket, std::vector<std::string> openPortMessages) {
	std::vector<int> hiddenPorts;

	for(std::vector<std::string>::iterator it = openPortMessages.begin(); it != openPortMessages.end(); it++) {
		if(!it->find("This is the port:")) {
			// Find the last 4 characters in the string (the port number)
			int port = atoi(it->substr(it->length() - 4).c_str());
			hiddenPorts.push_back(port);

			std::cout << port << std::endl;
		}
		
		else if(*it == "I only speak with fellow evil villains. (https://en.wikipedia.org/wiki/Evil_bit)") {
			std::cout << "Evil" << std::endl;
		}

		else if(*it == "Please send me a message with a valid udp checksum with value of 61453") {
			std::cout << "Checksum" << std::endl;
		}
	}

	return hiddenPorts;
}