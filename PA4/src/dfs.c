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
#include <sys/wait.h>
#include <libgen.h>

#include "../include/utils.h"
#include "../include/socket.h"

int main(int argc, char **argv)
{
    int sockfd;                                        /* socket */
    int connfd;                                        /* connection */
    int portno;                                        /* port to listen on */
    socklen_t clientlen;                               /* byte size of client's address */
    struct sockaddr_in serveraddr;                     /* server's addr */
    struct sockaddr_in clientaddr;                     /* client addr */
    char *requestMemory = (char *)malloc(BUFFER_SIZE); /* message request */
    char *request = requestMemory;
    int optval; /* flag value for setsockopt */
    char *dir;  /* directory to store files */

    /*
     * check command line arguments
     */
    if (argc != 3)
    {
        fprintf(stderr, "usage: %s <directory> <port>\n", argv[0]);
        exit(1);
    }
    dir = argv[1];
    portno = atoi(argv[2]);

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
     * Setup Complete. Start to serve requests
     * Some code adapted from geeksforgeeks.org/tcp-server-client-implementation-in-c/
     */

    if ((listen(sockfd, 5)) != 0)
    {
        error("Listen failed...\n");
    }
    else
        printf("Listening on port %d..\n", portno);

    int pid;

    while (1)
    {

        // Accept the data packet from client and verification
        connfd = accept(sockfd, (struct sockaddr *)&clientaddr, &clientlen);
        if (connfd < 0)
        {
            error("server accept failed...\n");
        }

        // if ((pid = fork()) == -1)
        // {
        //     close(connfd);
        //     continue;
        // }
        // else if (pid > 0)
        // {
        //     // I'm the parent
        //     close(connfd);            // child owns this connection now
        //     signal(SIGCHLD, SIG_IGN); // ignore child's signal, reap automatically
        //     continue;
        // }
        // else if (pid == 0)
        // {
        char cmd[100];
        char filename[100];
        int chunkSize;

        // I'm the child. Service the request. Parse the first line to figure out where to start
        request2buffer(connfd, request, BUFFER_SIZE);
        parseRequest(&request, cmd, filename, &chunkSize);

        if (strcmp(cmd, "PUT") == 0)
        {
            // put command recieved from client.
            serverPutFile(request, cmd, dir, filename, chunkSize);
        }
        else if (strcmp(cmd, "GET") == 0)
        {
            // for get, we don't want to jump ahead
            request = requestMemory;
            char *response = (char *)malloc(BUFFER_SIZE);
            char *path = pathConcat(dir, filename);
            FILE *fp = fopen(path, "rb");
            long fileSize = putFileInBuffer(response, BUFFER_SIZE, fp);
            writeToSocket(connfd, response, fileSize);
        }

        // close the connection after request is serviced
        close(connfd);

        // free memory
        free(requestMemory);

        break; // break out of the infinite loop and exit
        // }
    }
}