#ifndef SERVER_H
#define SERVER_H

#include <string>

void processClientRequest(int client_socket); 
void startServer(int port);  

#endif
