#include <iostream>
#include "Server.h"

int main(int argc, char* argv[]) {
    /* port */
    int port;

    /*check input */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    port = atoi(argv[1]);

    /*create new Server instance */
    Server *newServer = new Server(port);

    /*start new Server */
    newServer->Start();
}