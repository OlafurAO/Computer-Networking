#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sstream>
#include <string.h>

#define IP_ADDRESS "127.0.0.1"
#define PORT 5000

int connectToServer();
void executeCommand(int& socket, std::string cmd);
void receiveServerOutput(int& clientSocket);

int main() {
	std::cout << "Connecting to server" << std::endl;
	std::string cmd;
	int clientSocket;

	try {
                clientSocket = connectToServer();
	} catch(int a) {
	        std::cout << "Could not connect to server! :(" << std::endl;
                return 0;
	}

	while(cmd != "q" && cmd !=  "quit") {
                std::cout << ">>$";
                getline(std::cin, cmd);
                executeCommand(clientSocket, cmd);
                receiveServerOutput(clientSocket);
	}

	std::cout << "Disconnecting" << std::endl;
}

// Establishes a connection to a server and returns the socket used
int connectToServer() {
        // Initialize a socket with which to connect to the server
        int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in address;

        address.sin_family = AF_INET;
        address.sin_port = htons(PORT); // Port 5000

        // Convert the IP address into binary form
        inet_pton(AF_INET, IP_ADDRESS, &address.sin_addr); // IP address 127.0.0.1

        // Cast address to sockaddr pointer and try to connect to the server through the socket
        // If the client is unable to connect, it throws an exception
        if(connect(clientSocket, (sockaddr*) &address, sizeof(address)) < 0) {
                throw -1;
        } else {
                std::cout << "Connection successful" << std::endl;
        }

        return clientSocket;
}

// Sends the command to the server
void executeCommand(int& socket, std::string cmd) {
        if(send(socket,  cmd.c_str(), cmd.size() + 1, 0) < 0) {
                perror("Error executing command!");
        }
}

//  Receives the output of the command from the server and prints it out
void receiveServerOutput(int& clientSocket) {
        char buffer[10000];

        // Removes the previous output from the buffer
        memset(buffer, 0, sizeof(buffer));
        // Receive the output from the server and put it into the buffer
        recv(clientSocket, buffer, 10000, 0);

        std::cout << buffer << std::endl;
}
