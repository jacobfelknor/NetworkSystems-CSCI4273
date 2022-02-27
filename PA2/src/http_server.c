#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/limits.h>
#include <stdbool.h>

#include "../include/utils.h"
#include "../include/str_utils.h"

int main(int argc, char **argv)
{
    int sockfd;                    /* socket */
    int connfd;                    /* connection */
    int portno;                    /* port to listen on */
    int clientlen;                 /* byte size of client's address */
    struct sockaddr_in serveraddr; /* server's addr */
    struct sockaddr_in clientaddr; /* client addr */
    struct hostent *hostp;         /* client host info */
    char buf[BUFSIZ];              /* message buf */
    char *hostaddrp;               /* dotted decimal host addr string */
    int optval;                    /* flag value for setsockopt */
    int n;                         /* message byte size */

    /*
     * check command line arguments
     */
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    portno = atoi(argv[1]);

    /*
     * socket: create the parent socket
     */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    /* setsockopt: Handy debugging trick that lets
     * us rerun the server immediately after we kill it;
     * otherwise we have to wait about 20 secs.
     * Eliminates "ERROR on binding: Address already in use" error.
     */
    optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
               (const void *)&optval, sizeof(int));

    /*
     * build the server's Internet address
     */
    bzero((char *)&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)portno);

    /*
     * bind: associate the parent socket with a port
     */
    if (bind(sockfd, (struct sockaddr *)&serveraddr,
             sizeof(serveraddr)) < 0)
        error("ERROR on binding");

    clientlen = sizeof(clientaddr);

    /*
     * Setup Complete. Start to serve web requests
     * Some code adapted from geeksforgeeks.org/tcp-server-client-implementation-in-c/
     */

    if ((listen(sockfd, 5)) != 0)
    {
        error("Listen failed...\n");
    }
    else
        printf("Server listening..\n");

    // Accept the data packet from client and verification
    connfd = accept(sockfd, (struct sockaddr *)&clientaddr, &clientlen);
    if (connfd < 0)
    {
        error("server accept failed...\n");
    }
    else
        printf("accepted connection...\n");

    // copy request string into our buffer
    request2buffer(connfd, buf, BUFSIZ);

    // parse first line of request for our 3 main substrings (with conservatively long buffers...)
    char requestType[20];  // e.g. GET, POST, etc
    char requestPath[260]; // e.g. /some/dir/page.html
    char httpVersion[20];  // e.g. HTTP/1.1

    splitRequestString(buf, requestType, requestPath, httpVersion);

    printf("%s\n%s\n%s\n", requestType, requestPath, httpVersion);

    // close socket when done
    close(sockfd);
}