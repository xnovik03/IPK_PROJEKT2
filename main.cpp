#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include "TcpChatClient.h"  
#include "UdpChatClient.h"
#include "ChatClient.h"
#include <csignal>
#include <cstdlib>
#include "debug.h"

// Global pointer to ChatClient (common interface for TCP and UDP clients)
ChatClient* globalClient = nullptr;

// Signal handler to catch Ctrl+C (SIGINT) and send BYE message before exiting
void signalHandler(int signal) {
    (void)signal; 
    std::cerr << "SIGINT received!" << std::endl;
    if (globalClient) {
        std::cerr << "Sending BYE message...\n";
        globalClient->sendByeMessage();
    }
printf_debug("Total retransmissions: %d", totalRetransmissions);
    std::exit(0);
}

// Prints usage help to the console
void printHelp() {
    std::cout << "Usage: ./ipk25chat-client -t <tcp|udp> -s <server> [-p port] [-d timeout_ms] [-r retries] [-h]\n";
    std::cout << "  -t      Transport protocol: tcp or udp (REQUIRED)\n";
    std::cout << "  -s      Server hostname or IP (REQUIRED)\n";
    std::cout << "  -p      Server port (REQUIRED)\n";  
    std::cout << "  -d      UDP confirmation timeout in ms (default: 250)\n";
    std::cout << "  -r      UDP retries (default: 3)\n";
    std::cout << "  -h      Show this help message\n";
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, signalHandler); // Set up signal handling for Ctrl+C
    int timeoutMs = 250;     // Default: 250 ms
    int retries = 3; 
    std::string transport;  // Protocol type: tcp or udp
    std::string server;     // Server address
    int port = DEFAULT_PORT;        // Port number
    bool portSet = false;   // Flag to check if port is provided
     
    // Parse command-line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-t" && i + 1 < argc) transport = argv[++i];
        else if (arg == "-s" && i + 1 < argc) server = argv[++i];
        else if (arg == "-p" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
            portSet = true;
        }
           else if (arg == "-d" && i + 1 < argc) timeoutMs = std::stoi(argv[++i]);  // ✅ new
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

// Check if required arguments are missing
if (transport.empty() || server.empty()) {
    std::cerr << "ERROR: Missing required arguments (-t and -s are required).\n";
    printHelp();
    return 1;
}

// TCP: port optional, default to DEFAULT_PORT
if (transport == "tcp" && !portSet) {
    std::cerr << "No port specified for TCP. Using default port: " << DEFAULT_PORT << std::endl;
    port = DEFAULT_PORT;
}

// UDP: port required
if (transport == "udp" && !portSet) {
    std::cerr << "ERROR: Missing required port (-p) for UDP.\n";
    printHelp();
    return 1;
}

    // TCP client flow
    if (transport == "tcp") {
        TcpChatClient client(server, port);
        globalClient = &client; 
        if (!client.connectToServer()) return 1;
        client.run();
    }

    // UDP client flow
    else if (transport == "udp") {
    UdpChatClient udpClient(server, port, timeoutMs, retries);
        globalClient = &udpClient;
        if (!udpClient.connectToServer()) return 1;
        udpClient.run();
    }

    // Clean up and exit
    delete globalClient;
    return 0;
}
