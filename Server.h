//
// Created by Yuriy Mikhaildi on 11/18/21.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
#ifndef PROXY_SERVER_H
#define PROXY_SERVER_H

#include <unistd.h>      /* for read, write */
#include <sys/socket.h>  /* for socket use */
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <dirent.h>
#include <fcntl.h>

#define MAXLINE  8192  /* max text line length */
#define MAXBUF   2000  /* max I/O buffer size */
#define LISTENQ  1024  /* second argument to listen() */
#define HTTP "http://%[^/]"
#define NETPORT 80
#define HOST "\r\nHOST: %s\r\n"


struct ParsedURL {
    char *path;
    char *host;
    char *port;
};

class Server {
    int port;

public:

    explicit Server(int PortNumber) {
        this->port = PortNumber;
    }

    ~Server() = default;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // EXECUTION BLOCK //


    static void startDownStream(int connfd, char *ClientReq) {
//        size_t writer;
//        strcat(ClientReq, "\r\n\r\n");
//        if (!clientType) {
//            std::cout << "Telnet" << std::endl;
//            std::cout << "Adding new line: " << ClientReq << "|" << std::endl;
//            writer = send(serverconnfd, ClientReq, strlen(ClientReq), 0);
//            if (writer < 0) {
//                std::cout << "Failed to write req to server" << std::endl;
//                handle400error(connfd);
//            }
//
//        } else {
//            std::cout << "Browser" << std::endl;
//            char *fullrequest = (char *) malloc(strlen(hostname) + 30);
//            sprintf(fullrequest, "GET http://%s\n HTTP/1.1\r\n", hostname);
//            writer = send(serverconnfd, fullrequest, strlen(fullrequest), 0);
//            if (writer < 0) {
//                std::cout << "Failed to write req to server" << std::endl;
//                handle400error(connfd);
//            }
//        }
//
//        char responseFromProxy[10001];
//        responseFromProxy[10000] = '\0';
//        memset(responseFromProxy, 0, 10000);
//
//        size_t n;
//        while (true) {
//
//            n = recv(serverconnfd, responseFromProxy, 10000, 0);
//
//            if (n < 0) {
//                std::cout << "Failed to receive the page" << std::endl;
//                close(serverconnfd);
//                handle400error(connfd);
//            }
//
//            handleClientResponse(connfd, responseFromProxy);
//
//            memset(responseFromProxy, 0, 10000);
//        }

    }

    static void *executeUpStream(int connfd) {
        std::cout << "Executing Data UpStream." << std::endl;
        size_t read_bytes;
        char ClientReq[MAXBUF];
        while ((read_bytes = read(connfd, ClientReq, MAXBUF)) > 0) {
            /* handle error */
            if (read_bytes < 0) {
                handle400error(connfd);
            }
            std::cout << "Request Processing..." << std::endl;
            char ClientReqCopy[MAXLINE];
            strncpy(ClientReqCopy, ClientReq, MAXLINE);

            /*check browser or telnet */
            bool clientType = false;

            if (strstr(ClientReq, "Host:") != NULL)
                clientType = true;

            /*parse the request */
            char *request_type = strtok(ClientReqCopy, " ");
            char *url = strtok(NULL, " ");
            char *http_version = strtok(NULL, "\r");

            char hostname[MAXLINE];
            sscanf(url, HTTP, hostname);

            /* check request type */
            if (strcmp(request_type, "GET") != 0) {
                std::cout << "Request type is wrong" << std::endl;
                handle400error(connfd);
            }
            if (url == NULL) {
                std::cout << "URL version is wrong or missing" << std::endl;
                handle400error(connfd);
            }
            if (http_version == NULL) {
                std::cout << "HTTP version is wrong or missing" << std::endl;
                handle400error(connfd);
            }

            std::cout << "Request Host: " << hostname << std::endl;
            struct hostent *ServerHost = gethostbyname(hostname);

            in_addr* address = (in_addr*) ServerHost->h_addr;
            std::string ip_address = inet_ntoa(* address);

            std::cout << "Resolved IP: " << "(" << ip_address << ")" << std::endl;

            struct sockaddr_in r_addr;
            bzero((char *) &r_addr, sizeof(r_addr));
            bcopy((char* ) ServerHost->h_addr,
                  (char* ) &r_addr.sin_addr.s_addr, ServerHost->h_length);
            r_addr.sin_family = AF_INET;
            r_addr.sin_port = htons(80);

            int r_sock = socket(AF_INET, SOCK_STREAM, 0);

            if((connect(r_sock, (struct sockaddr *) &r_addr, sizeof(r_addr))) < 0){
                std::cout << "Socket Connect Fail" << std::endl;
                handle500error(connfd);
            }
            char hostBuf[MAXBUF];
            sprintf(hostBuf, HOST, hostname);
            strcat(ClientReq, hostBuf);

            std::cout << "Attaching host... \n" << ClientReq << std::endl;

            size_t bytesSent = write(r_sock, ClientReq, strlen(ClientReq));

            if (bytesSent < 0) {
                std::cout << "Failed to write req to server" << std::endl;
                handle400error(connfd);
            }
            size_t bytesRead;
            char responseFromProxy[1024];
            do{
                bytesRead = read(r_sock, responseFromProxy, sizeof(responseFromProxy) - 1);
                if(bytesRead < 0){
                    std::cout << "Failed to read response from web:\n" << responseFromProxy << std::endl;
                    handle400error(connfd);
                }
                responseFromProxy[bytesRead] = 0;
                bytesSent = write(connfd, responseFromProxy, bytesRead);
                if(bytesSent < 0){
                    std::cout << "Failed to send response to client:\n" << responseFromProxy << std::endl;
                    handle400error(connfd);
                }
                bzero(responseFromProxy, strlen(responseFromProxy));

            }while(bytesRead > 0);

            bzero(ClientReq, strlen(ClientReq));
            bzero(hostBuf, strlen(hostBuf));
        }

        return nullptr;
    }

    void *Start() {
        int listenfd, *connfdp;
        socklen_t clientlen = sizeof(struct sockaddr_in);
        sockaddr *clientaddr = nullptr;
        pthread_t tid;
        listenfd = open_listenfd(this->port);
        std::cout << "Proxy Started." << std::endl;
        while (true) {
            connfdp = (int *) (malloc(sizeof(int)));
            *connfdp = accept(listenfd, clientaddr, (socklen_t *) clientlen);
            pthread_create(&tid, NULL, create_thread, connfdp);

        }

    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // HELPER BLOCK //
    /*
     * open_listenfd - open and return a listening socket on PortNumber
     * Returns -1 in case of failure
     */
    static int open_listenfd(int PortNumber) {
        int listenfd, optval = 1;
        struct sockaddr_in serveraddr;

        /* Create a socket descriptor */
        if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            return -1;

        /* Eliminates "Address already in use" error from bind. */
        if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
                       (const void *) &optval, sizeof(int)) < 0)
            return -1;

        /* listenfd will be an endpoint for all requests to PortNumber
           on any IP address for this host */
        bzero((char *) &serveraddr, sizeof(serveraddr));
        serveraddr.sin_family = AF_INET;
        serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
        serveraddr.sin_port = htons((unsigned short) PortNumber);
        if (bind(listenfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
            return -1;

        /* Make it a listening socket ready to accept connection requests */
        if (listen(listenfd, LISTENQ) < 0)
            return -1;
        return listenfd;
    } /* end open_listenfd */

    static void *create_thread(void *vargp) {
        int connfd = *((int *) vargp);
        pthread_detach(pthread_self());
        free(vargp);
        executeUpStream(connfd);
        close(connfd);
        return nullptr;
    }

    static void handle400error(int connfd) {
        char error_buf[MAXLINE];
        std::cout << "\n\"HTTP/1.1 400 Bad Request\"\n" << std::endl;
        strcpy(error_buf, "HTTP/1.1 400 Bad Request");
        send(connfd, error_buf, strlen(error_buf), 0);
    }

    static void handle500error(int connfd) {
        char error_buf[MAXLINE];
        std::cout << "\n\"HTTP/1.1 500 Internal Fail\"\n" << std::endl;
        strcpy(error_buf, "HTTP/1.1 500 Internal Fail");
        send(connfd, error_buf, strlen(error_buf), 0);
    }

    static void handle200answer(int connfd) {
        char error_buf[MAXLINE];
        std::cout << "\n\"HTTP/1.1 200 Proxy Accepted Request\"\n" << std::endl;
        strcpy(error_buf, "HTTP/1.1 200 Proxy Accepted Request");
        send(connfd, error_buf, strlen(error_buf), 0);
    }

    // parser function based of https://stackoverflow.com/questions/43906956/split-url-into-host-port-and-resource-c
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////



};


#endif //PROXY_SERVER_H

#pragma clang diagnostic pop