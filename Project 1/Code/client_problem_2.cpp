#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <arpa/inet.h>

#define IP_ADDRESS "130.208.243.61"
#define PORT 4097

int connectToServer();
void executeCommand(int& socket, std::string cmd);

int main() {
        std::string cmd = "SYS who";

	std::cout << "Connecting to server" << std::endl;
	int clientSocket = connectToServer();
	std::cout << "Executing command " << cmd << std::endl;
	executeCommand(clientSocket, cmd);
}

int connectToServer() {
        //https://www.youtube.com/watch?v=fmn-pRvNaho
        int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in address;

        address.sin_family = AF_INET;
        address.sin_port = htons(PORT);
        inet_pton(AF_INET, IP_ADDRESS, &address.sin_addr);

        if(connect(clientSocket, (sockaddr*) &address, sizeof(address)) < 0) {
                perror("Could not connect to server! :(");
        } else {
                std::cout << "Connection successful" << std::endl;
        }

        return clientSocket;
}

void executeCommand(int& socket, std::string cmd) {
        if(send(socket,  cmd.c_str(), cmd.size() + 1, 0) < 0) {
                perror("Error executing command!");
        } else {
                std::cout << "Command executed" << std::endl;
        }
}
