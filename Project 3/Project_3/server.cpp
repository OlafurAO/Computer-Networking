#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <algorithm>
#include <map>
#include <vector>
#include <ctime>

#include <iostream>
#include <sstream>
#include <fstream>
#include <fstream>
#include <thread>
#include <map>

#include <unistd.h>

#define IP_ADDRESS "130.208.243.61"
#define BACKLOG  5          // Allowed length of queue of waiting connections
#define MAX_CLIENTS 1

// Simple class for handling connections from clients.
//
// Client(int socket) - socket to send/receive traffic from client.
class Client {
  public:
    int sock;              // socket of client connection
    std::string name;           // Limit length of name of client's user

    Client(int socket) : sock(socket){} 

    ~Client(){}            // Virtual destructor defined for base class
};

class Server {
public:
	Server(int socket, std::string name, std::string ip, std::string port) 
		: socket(socket), name(name), ip(ip), port(port) {}
	~Server() {}

	void saveMessage(std::string msg) {
		serverMessages.push_back(msg);
	}

	std::vector<std::string> serverMessages;

	int socket;
	std::string name;
	std::string ip;
	std::string port;	
};

std::map<int, Client*> clients; // Lookup table for per Client information
std::map<int, Server*> servers;
int serverCount = 0;			// Current number of servers connected
int clientCount = 0;

fd_set openSockets;             // Current open sockets 
fd_set readSockets;             // Socket list for select()        
fd_set exceptSockets;           // Exception socket list

int open_client_socket(int portno); // Open socket for specified port.
				
void connectToServer(std::string name, std::string ip, std::string port, bool expand);
void connectToServer();
// See if a server that this server is newly connected to is connected to any other servers.
// If so, this server will try to connect to them as well
void expandServerConnection(int serverSock);
void closeClient(int clientSocket, fd_set *openSockets, int *maxfds); // Close a client's connection and remove it from the client list
void clientCommand(int clientSocket, fd_set *openSockets, int *maxfds, char *buffer, bool server); // Process command from client on the server

/////////////////////////// SERVER COMMANDS
void CONNECT(int clientSocket, std::vector<std::string> tokens);
void WHO(int clientSocket);
void MSG_ALL(int clientSocket, std::vector<std::string> tokens);
void MSG(int clientSocket, std::vector<std::string> tokens);
void LISTSERVERS(int clientSocket);
void GET_MSG_GROUP(int clientSocket, std::vector<std::string> tokens);
void SEND_MSG_GROUP(std::vector<std::string> tokens);
void KEEPALIVE();
//////////////////////////////////////////////

// Returns current local time in hours, minutes and seconds
std::string getCurrentTime();
std::string getHour(std::tm* time);
std::string getMin(std::tm* time);
std::string getSec(std::tm* time);

int main(int argc, char* argv[]) {
	struct sockaddr_in client;

    bool finished;
    int listenSock;                 // Socket for connections to server
    int clientSock;                 // Socket of connecting client
	int maxfds;                     // Passed to select() as max fd in set

    socklen_t clientLen;
    char buffer[1025];              // buffer for reading from clients

	if (argc > 2) {
		connectToServer("TEMP_1", argv[2], argv[3], true);
	}

	servers[0] = new Server(0, "P3_GROUP_70", IP_ADDRESS, argv[1]);

    // Setup socket for server to listen to;
    listenSock = open_client_socket(atoi(argv[1]));

    printf("Listening on port: %d\n", listenSock);

    if(listen(listenSock, BACKLOG) < 0) {
        printf("Listen failed on port %s\n", argv[1]);
        exit(0);
    } 
	else {
        FD_SET(listenSock, &openSockets);  // Add listen socket to socket set we are monitoring
        maxfds = listenSock;
    }

    finished = false;

    while(!finished) {
        // Get modifiable copy of readSockets
        readSockets = exceptSockets = openSockets;
        memset(buffer, 0, sizeof(buffer));

        // Look at sockets and see which ones have something to be read()
        int n = select(maxfds + 1, &readSockets, NULL, &exceptSockets, NULL);

        if(n < 0) {
            perror("select failed - closing down\n");
            finished = true;
        }
        else {
            // First, accept  any new connections to the server on the listening socket
            if(FD_ISSET(listenSock, &readSockets)) {
				clientSock = accept(listenSock, (struct sockaddr *)&client, &clientLen);

               // Add new client to the list of open sockets
               FD_SET(clientSock, &openSockets);

               // And update the maximum file descriptor
               maxfds = std::max(maxfds, clientSock);

			   if (clientCount < MAX_CLIENTS) {
				   // create a new client to store information.
				   //send(clientSock, "P3_GROUP_70", sizeof("P3_GROUP_70"), 0);
				   clients[clientSock] = new Client(clientSock);
				   clientCount++;
				   
				   printf("Client connected on server: %d\n", clientSock);
			   } else {
				   if (serverCount < BACKLOG) {
					   //send(clientSock, "P3_GROUP_70", sizeof("P3_GROUP_70"), 0);
					   
					   std::string ip = inet_ntoa(client.sin_addr);
					   std::string port = std::to_string(ntohs(client.sin_port));
					   servers[clientSock] = new Server(clientSock, "TEMP_1", ip, port);
					   serverCount++;

					   std::cout << "\n(" << getCurrentTime() << "): ";
					   printf("Server connected: \n");
					   std::cout << "IP: " << ip << std::endl;
					   std::cout << "PORT: " << port << "\n" << std::endl;
					   //connectToServer("TEMP_1", ip, port, true);
				   } else {
					   std::cout << "Maximum server capacity reached" << std::endl;
				   }
			   }

               // Decrement the number of sockets waiting to be dealt with
               n--;
            }
            // Now check for commands from clients
            while(n-- > 0) {
               for(auto const& pair : clients) {
                  Client *client = pair.second;

                  if(FD_ISSET(client->sock, &readSockets)) {
                      // recv() == 0 means client has closed connection
                      if(recv(client->sock, buffer, sizeof(buffer), MSG_DONTWAIT) == 0) {
                          printf("Client closed connection: %d", client->sock);
                          close(client->sock);      

                          closeClient(client->sock, &openSockets, &maxfds);
                      }
                      // We don't check for -1 (nothing received) because select()
                      // only triggers if there is something on the socket for us.
                      else {
                          std::cout << buffer << std::endl;
                          clientCommand(client->sock, &openSockets, &maxfds, buffer, false);
                      }
                  }
               }
            }
        }
    }
}

int open_client_socket(int portno) {
	struct sockaddr_in sk_addr;   // address settings for bind()
	int sock;                     // socket opened for this port
	int set = 1;                  // for setsockopt

	// Create socket for connection. Set to be non-blocking, so recv will
	// return immediately if there isn't anything waiting to be read.

	if ((sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0) {
		perror("Failed to open socket");
		return(-1);
	}

	struct timeval timeout;
	timeout.tv_sec = 0.9;
	timeout.tv_usec = 1;

	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

	// Turn on SO_REUSEADDR to allow socket to be quickly reused after 
	// program exit.

	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set)) < 0) {
		perror("Failed to set SO_REUSEADDR:");
	}

	memset(&sk_addr, 0, sizeof(sk_addr));

	sk_addr.sin_family = AF_INET;
	sk_addr.sin_addr.s_addr = INADDR_ANY;
	sk_addr.sin_port = htons(portno);

	// Bind to socket to listen for connections from clients

	if (bind(sock, (struct sockaddr *)&sk_addr, sizeof(sk_addr)) < 0) {
		perror("Failed to bind to socket:");
		return(-1);
	} else {
		return(sock);
	}
}

void connectToServer(std::string name, std::string ip, std::string port, bool expand) {
	if (serverCount >= BACKLOG) {
		return;
	}
	struct addrinfo hints, *svr;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	memset(&hints, 0, sizeof(hints));

	if (getaddrinfo(ip.c_str(), port.c_str(), &hints, &svr) != 0) {
		perror("getaddrinfo failed: ");
		exit(0);
	}

	int serverSock = socket(svr->ai_family, svr->ai_socktype, svr->ai_protocol);
	int set = 1;

	if (setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set)) < 0) {
		perror("setsockopt failed: ");
		exit(0);
	}

	if (connect(serverSock, svr->ai_addr, svr->ai_addrlen) < 0) {
		perror("Connect failed: ");
		exit(0);
	}

	//send(serverSock, "P3_GROUP_70", sizeof("P3_GROUP_70"), 0);
	
	servers[serverSock] = new Server(serverSock, name, ip, port);
	serverCount++;

	if (expand) {
		expandServerConnection(serverSock);
	}
}

void expandServerConnection(int serverSock) {
	// Check if the newly connected server is connected to other servers
	// and attempt to connect to them
	//if (serverCount < BACKLOG) {
		if (send(serverSock, "\1LISTSERVERS, P3_GROUP_70\4", sizeof("\1LISTSERVERS, P3_GROUP_70\4"), 0) > 0) {
			char buffer[1025]; 
			memset(buffer, 0, sizeof(buffer));

			int nread = read(serverSock, buffer, sizeof(buffer));

			std::cout << "PAPAPA" << std::endl;

			if (nread > 0) {
				std::string buff = std::string(buffer);

				if (buff.find_first_not_of(' ') == std::string::npos) {
					return;
				}

				std::cout << buff << std::endl;

				std::vector<std::string> serverList;
				int serverIndex = 8;    // Where the first server starts in the buffer. Omit the "SERVER, " part

				for (int i = 8; i < buff.size(); i++) {
					if (buff[i] == ';') {
						serverList.push_back(buff.substr(serverIndex, buff.find(";")));
						serverIndex = i;
					}
				}

				for (int i = 0; i < serverList.size(); i++) {
					if (i == 0) {
						std::string srvName = serverList[i].substr(0, serverList[i].find(","));
						// Replace the TEMP_1 name with the actual name
						servers[serverSock]->name = srvName;
					} else {
						size_t stringIndex = serverList[i].find(",");
						std::string srvName = serverList[i].substr(0, stringIndex);
						serverList[i].erase(0, srvName.length() + 1);

						stringIndex = serverList[i].find(",");
						std::string srvIP = serverList[i].substr(0, stringIndex);
						serverList[i].erase(0, srvIP.length() + 1);

						stringIndex = serverList[i].find(";");
						std::string srvPort = serverList[i].substr(0, stringIndex);
						serverList[i].erase(0, srvPort.length() + 1);

						srvName.erase(std::remove(srvName.begin(), srvName.end(), ';'), srvName.end());
						srvName.erase(std::remove(srvName.begin(), srvName.end(), ','), srvName.end());
						srvName.erase(std::remove(srvName.begin(), srvName.end(), ' '), srvName.end());

						srvIP.erase(std::remove(srvIP.begin(), srvIP.end(), ';'), srvIP.end());
						srvIP.erase(std::remove(srvIP.begin(), srvIP.end(), ','), srvIP.end());
						srvIP.erase(std::remove(srvIP.begin(), srvIP.end(), ' '), srvIP.end());

						srvPort.erase(std::remove(srvPort.begin(), srvPort.end(), ';'), srvPort.end());
						srvPort.erase(std::remove(srvPort.begin(), srvPort.end(), ','), srvPort.end());
						srvPort.erase(std::remove(srvPort.begin(), srvPort.end(), ' '), srvPort.end());

						connectToServer(srvName, srvIP, srvPort, true);
					}
				}
			}
		//}
	}
}

void closeClient(int clientSocket, fd_set *openSockets, int *maxfds) {
	// Remove client from the clients list
	clients.erase(clientSocket);

	// If this client's socket is maxfds then the next lowest
	// one has to be determined. Socket fd's can be reused by the Kernel,
	// so there aren't any nice ways to do this.

	if (*maxfds == clientSocket)
	{
		for (auto const& p : clients)
		{
			*maxfds = std::max(*maxfds, p.second->sock);
		}
	}

	// And remove from the list of open sockets.
	FD_CLR(clientSocket, openSockets);
	clientCount--;
}

void clientCommand(int clientSocket, fd_set *openSockets, int *maxfds, char *buffer, bool server) {
	std::vector<std::string> tokens;
	std::string token;

	// Split command from client into tokens for parsing
	std::stringstream stream(buffer);

	while (stream >> token) {
		tokens.push_back(token);
	}

	if ((tokens[0].compare("CONNECT") == 0) && (tokens.size() == 2)) {
		CONNECT(clientSocket, tokens);
	}

	else if (tokens[0].compare("LEAVE") == 0) {
		// Close the socket, and leave the socket handling
		// code to deal with tidying up clients etc. when
		// select() detects the OS has torn down the connection.

		closeClient(clientSocket, openSockets, maxfds);
	}

	else if (tokens[0].compare("WHO") == 0) {
		WHO(clientSocket);
	}

	// This is slightly fragile, since it's relying on the order
	// of evaluation of the if statement.
	else if ((tokens[0].compare("MSG") == 0) && (tokens[1].compare("ALL") == 0)) {
		MSG_ALL(clientSocket, tokens);
	}

	else if (tokens[0].compare("MSG") == 0) {
		MSG(clientSocket, tokens);
	}

	// Return a list of connected servers
	else if (tokens[0].find("LISTSERVERS") != std::string::npos) {
		LISTSERVERS(clientSocket);
	}

	else if (tokens[0].compare("GET_MSG,") == 0) {
		GET_MSG_GROUP(clientSocket, tokens);
	}

	else if (tokens[0].compare("SEND_MSG,") == 0) {
		SEND_MSG_GROUP(tokens);
	}

	else {
		std::cout << "Unknown command from client:" << buffer << std::endl;
	}
}

//////////////////////// SERVER COMMANDS
void CONNECT(int clientSocket, std::vector<std::string> tokens) {
	clients[clientSocket]->name = tokens[1];
	std::string joinMsg = "\033[1;35m" + std::string(tokens[1]) + " has joined the chat\033[0m";
	for (auto const& pair : clients) {
		send(pair.second->sock, joinMsg.c_str(), joinMsg.length(), 0);
	}
}

void WHO(int clientSocket) {
	std::cout << "Who is logged on" << std::endl;
	std::string msg;
	for (auto const& names : clients) {
		msg += names.second->name + ",";
	}

	// Reducing the msg length by 1 loses the excess "," - which
	// granted is totally cheating.
	send(clientSocket, msg.c_str(), msg.length() - 1, 0);
}

void MSG_ALL(int clientSocket, std::vector<std::string> tokens) {
	std::string timeStamp = getCurrentTime();
	std::string msg = std::string("\033[1;31m") + "(" + timeStamp + ") \033[0m" + "\033[1;32m"
		+ clients[clientSocket]->name + "\033[0m" + ": ";

	msg.erase(std::remove(msg.begin(), msg.end(), '\n'), msg.end()); // Remove the newline from currentTime

	for (auto i = tokens.begin() + 2; i != tokens.end(); i++) {
		msg += *i + " ";
	}

	for (auto const& pair : clients) {
		send(pair.second->sock, msg.c_str(), msg.length(), 0);
	}

	servers[0]->saveMessage(msg);
}

void MSG(int clientSocket, std::vector<std::string> tokens) {
	for (auto const& pair : clients) {
		if (pair.second->name.compare(tokens[1]) == 0) {
			std::time_t currentTime = std::time(0);
			std::tm* time = std::localtime(&currentTime);

			std::string hour = getHour(time);
			std::string min = getMin(time);
			std::string sec = getSec(time);

			std::string timeStamp = hour + ":" + min + ":" + sec;
			std::string msg = std::string("\033[1;31m") + "(" + timeStamp + ") \033[0m" + "\033[1;32m" 
				+ clients[clientSocket]->name + "\033[0m" + " (private): ";

			msg.erase(std::remove(msg.begin(), msg.end(), '\n'), msg.end()); // Remove the newline from currentTime

			for (auto i = tokens.begin() + 2; i != tokens.end(); i++) {
				msg += *i + " ";
			}
			send(pair.second->sock, msg.c_str(), msg.length(), 0);
		}
	}
}

void LISTSERVERS(int clientSocket) {
	std::string serverList = "SERVERS, ";
	for (const auto &p : servers) {
		serverList += p.second->name + ", " + p.second->ip + ", " + p.second->port + "; ";
	}

	send(clientSocket, serverList.c_str(), serverList.length(), 0);
}

void GET_MSG_GROUP(int clientSocket, std::vector<std::string> tokens) {
	std::string groupID = tokens[1];
	int serverSocket = -1;

	for (const auto &p : servers) {
		if (p.second->name.find(groupID) != std::string::npos) {
			serverSocket = p.second->socket;
			break;
		}
	}

	if (serverSocket != -1) {
		std::string messages = "===Messages on this server: ===\n";
		std::vector<std::string> serverMessages = servers[serverSocket]->serverMessages;

		for (std::vector<std::string>::iterator it = serverMessages.begin(); it != serverMessages.end(); it++) {
			messages += *it + "\n";
		}

		send(clientSocket, messages.c_str(), messages.length(), 0);
	}
}

void SEND_MSG_GROUP(std::vector<std::string> tokens) {
	std::string groupID = tokens[1];
	int serverSocket = -1;

	for (const auto &p : servers) {
		if (p.second->name.find(groupID) != std::string::npos) {
			serverSocket = p.second->socket;
			break;
		}
	}

	if (serverSocket != -1) {
		std::string msg = "MSG ALL ";
		for (auto i = tokens.begin() + 2; i != tokens.end(); i++) {
			msg += *i + " ";
		}
		send(serverSocket, msg.c_str(), msg.length(), 0);
	}
}

void KEEPALIVE() {
	
}
//////////////////////////////////////////////

std::string getCurrentTime() {
	std::time_t currentTime = std::time(0);
	std::tm* time = std::localtime(&currentTime);

	std::string hour = getHour(time);
	std::string min = getMin(time);
	std::string sec = getSec(time);

	return hour + ":" + min + ":" + sec;
}

std::string getHour(std::tm* time) {
	std::string hour;
	if (time->tm_hour < 10) {
		hour = "0" + std::to_string(time->tm_hour);
	}
	else {
		hour = std::to_string(time->tm_hour);
	}
	return hour;
}

std::string getMin(std::tm* time) {
	std::string min;
	if (time->tm_min < 10) {
		min = "0" + std::to_string(time->tm_min);
	}
	else {
		min = std::to_string(time->tm_min);
	}
	return min;
}

std::string getSec(std::tm* time) {
	std::string sec;
	if (time->tm_sec < 10) {
		sec = "0" + std::to_string(time->tm_sec);
	}
	else {
		sec = std::to_string(time->tm_sec);
	}
	return sec;
}