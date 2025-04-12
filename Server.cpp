#include <iostream>
#include <string>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string>
#include "Server.h"

bool isValidAuth(const std::string& username, const std::string& password) {
    return (!username.empty() && !password.empty());
}

bool isValidChannel(const std::string& channel) {
    return !channel.empty();  // Kanál nesmí být prázdný
}

void processClientRequest(int client_socket) {
    char buffer[1024];
    ssize_t bytes_received;

    while ((bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        std::string command(buffer);

        
        std::cout << "Received command: " << command << std::endl; 

        if (command.find("AUTH") == 0) {
            std::string username = command.substr(5, command.find("AS") - 5);
            std::string password = command.substr(command.find("USING") + 6);

         
            std::cout << "Authenticating: Username = " << username << ", Password = " << password << std::endl;

            if (isValidAuth(username, password)) {
                send(client_socket, "REPLY OK IS Auth success.\r\n", strlen("REPLY OK IS Auth success.\r\n"), 0);
                std::cout << "Sent: REPLY OK IS Auth success." << std::endl;  
            } else {
                send(client_socket, "REPLY NOK IS Auth failed.\r\n", strlen("REPLY NOK IS Auth failed.\r\n"), 0);
                std::cout << "Sent: REPLY NOK IS Auth failed." << std::endl;  
            }
        }
        else if (command.find("JOIN") == 0) {
            std::string channel = command.substr(5);
            std::cout << "Joining channel: " << channel << std::endl;  

            if (isValidChannel(channel)) {
                send(client_socket, "REPLY OK IS Join success.\r\n", strlen("REPLY OK IS Join success.\r\n"), 0);
                std::cout << "Sent: REPLY OK IS Join success." << std::endl;  
            } else {
                send(client_socket, "REPLY NOK IS Join failed.\r\n", strlen("REPLY NOK IS Join failed.\r\n"), 0);
                std::cout << "Sent: REPLY NOK IS Join failed." << std::endl; 
            }
        }
        else if (command.find("RENAME") == 0) {
            std::string newDisplayName = command.substr(7);
            std::cout << "Renaming to: " << newDisplayName << std::endl;  

            if (!newDisplayName.empty()) {
                send(client_socket, "REPLY OK IS Rename success.\r\n", strlen("REPLY OK IS Rename success.\r\n"), 0);
                std::cout << "Sent: REPLY OK IS Rename success." << std::endl;  
            } else {
                send(client_socket, "REPLY NOK IS Rename failed.\r\n", strlen("REPLY NOK IS Rename failed.\r\n"), 0);
                std::cout << "Sent: REPLY NOK IS Rename failed." << std::endl;  
            }
        }
        else {
            send(client_socket, "REPLY NOK IS Invalid command.\r\n", strlen("REPLY NOK IS Invalid command.\r\n"), 0);
            std::cout << "Sent: REPLY NOK IS Invalid command." << std::endl;  
        }

        if (bytes_received == 0) {
            std::cout << "Client disconnected.\n";
            break;
        }
    }
}


void startServer(int port) {
    int server_fd, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "ERROR: Socket creation failed.\n";
        return;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "ERROR: Binding failed.\n";
        close(server_fd);
        return;
    }

    if (listen(server_fd, 5) == -1) {
        std::cerr << "ERROR: Listening failed.\n";
        close(server_fd);
        return;
    }

    std::cout << "Server listening on port " << port << "...\n";

    while ((client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &client_len)) != -1) {
        std::cout << "Client connected.\n";
        processClientRequest(client_socket);
        close(client_socket);
    }

    close(server_fd);
}
