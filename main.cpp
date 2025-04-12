#include <iostream>
#include <string>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cerrno>
#include <thread> 
#include "TcpChatClient.h"
#include "Server.h"

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

    // Zpracování argumentů pro příkazovou řádku
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-t" && i + 1 < argc) transport = argv[++i];
        else if (arg == "-s" && i + 1 < argc) server = argv[++i];
        else if (arg == "-p" && i + 1 < argc) port = std::stoi(argv[++i]);
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

    // Spuštění serveru v samostatném vlákně
    std::thread serverThread(startServer, port);
    serverThread.detach();  // Odsynchronizování vlákna

    if (transport == "tcp") {
        TcpChatClient client(server, port);
        if (!client.connectToServer()) return 1;
        client.run();
    }

    return 0;
}
