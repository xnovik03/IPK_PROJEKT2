#include <iostream>
#include <string>
#include <cstring>      // memset
#include <sys/types.h>
#include <sys/socket.h> // socket, connect, AF_INET, SOCK_STREAM
#include <netdb.h>      // getaddrinfo, addrinfo, freeaddrinfo
#include <unistd.h>     // close()
#include <cerrno>    
#include "TcpChatClient.h"
void printHelp() {
    std::cout << "Usage: ./ipk25chat-client -t <tcp|udp> -s <server> [-p port] [-d timeout_ms] [-r retries] [-h]\n";
    std::cout << "  -t      Transport protocol: tcp or udp (REQUIRED)\n";
    std::cout << "  -s      Server hostname or IP (REQUIRED)\n";
    std::cout << "  -p      Server port (default: 4567)\n";
    std::cout << "  -d      UDP confirmation timeout in ms (default: 250)\n";
    std::cout << "  -r      UDP retries (default: 3)\n";
    std::cout << "  -h      Show this help message\n";
}

int main(int argc, char* argv[]) {
    std::string transport;
    std::string server;
    
    int port = 4567;
    int timeout = 250;
    int retries = 3;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-t" && i + 1 < argc) transport = argv[++i];
        else if (arg == "-s" && i + 1 < argc) server = argv[++i];
        else if (arg == "-p" && i + 1 < argc) port = std::stoi(argv[++i]);
        else if (arg == "-d" && i + 1 < argc) timeout = std::stoi(argv[++i]);
        else if (arg == "-r" && i + 1 < argc) retries = std::stoi(argv[++i]);
        else if (arg == "-h") {
            printHelp();
            return 0;
        } else {
            std::cerr << "ERROR: Unknown or malformed argument: " << arg << "\n";
            printHelp();
            return 1;
        }
    }

    if (transport.empty() || server.empty()) {
        std::cerr << "ERROR: Missing required arguments (-t and -s).\n";
        printHelp();
        return 1;
    }

    if (transport == "tcp") {
        //Získání adresy serveru
        struct addrinfo hints{}, *res;
        hints.ai_family = AF_INET;       // IPv4
        hints.ai_socktype = SOCK_STREAM; // TCP

        int status = getaddrinfo(server.c_str(), std::to_string(port).c_str(), &hints, &res);
        if (status != 0) {
            std::cerr << "ERROR: getaddrinfo failed: " << gai_strerror(status) << "\n";
            return 1;
        }

        //Vytvoření socketu
        int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sockfd == -1) {
            std::perror("ERROR: socket creation failed");
            freeaddrinfo(res);
            return 1;
        }

        // Připojení na server
        if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
            std::perror("ERROR: connect failed");
            close(sockfd);
            freeaddrinfo(res);
            return 1;
        }

        std::cout << " Connected to " << server << ":" << port << " over TCP.\n";

        //Zavřeme spojení
        close(sockfd);
        freeaddrinfo(res);
    }

    return 0;
}