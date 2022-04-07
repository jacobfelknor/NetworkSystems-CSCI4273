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
#include "../include/str_utils.h"
#include "../include/forwarding.h"

int main(int argc, char **argv)
{
    int sockfd;                    /* socket */
    int connfd;                    /* connection */
    int portno;                    /* port to listen on */
    int clientlen;                 /* byte size of client's address */
    struct sockaddr_in serveraddr; /* server's addr */
    struct sockaddr_in clientaddr; /* client addr */
    struct hostent *hostp;         /* client host info */
    char request[BUFSIZ];          /* message request */
    char *hostaddrp;               /* dotted decimal host addr string */
    int optval;                    /* flag value for setsockopt */
    int n;                         /* message byte size */
    int cacheage;                  /* number of seconds to keep the cache */

    /*
     * check command line arguments
     */
    if (argc != 3)
    {
        fprintf(stderr, "usage: %s <port> <cache age>\n", argv[0]);
        exit(1);
    }
    portno = atoi(argv[1]);
    cacheage = atoi(argv[2]);

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

        if ((pid = fork()) == -1)
        {
            close(connfd);
            continue;
        }
        else if (pid > 0)
        {
            // I'm the parent
            close(connfd);            // child owns this connection now
            signal(SIGCHLD, SIG_IGN); // ignore child's signal, reap automatically
            continue;
        }
        else if (pid == 0)
        {
            // I'm the child. Service the request

            // copy request string into our buffer
            request2buffer(connfd, request, BUFSIZ);

            // parse first line of request for our 3 main substrings (with conservatively long buffers...)
            char requestMethod[20]; // e.g. GET, POST, etc
            char requestPath[260];  // e.g. /some/dir/page.html
            char httpVersion[20];   // e.g. HTTP/1.1
            char *responseBuffer = (char *)malloc(RESPONSE_BUFFER_SIZE);
            bzero(responseBuffer, RESPONSE_BUFFER_SIZE);
            long responseSize;

            splitRequestString(request, requestMethod, requestPath, httpVersion);

            if (requestIsValid(responseBuffer, requestPath, requestMethod, httpVersion, &responseSize))
            {

                // compute the md5 hash of the request path. This will allow us to look later if a
                // user has requested the same page
                char *md5path = computeMD5Path(requestPath);
                printf("    Looking for %s\n", md5path);
                FILE *readCachefp = fopen(md5path, "rb");
                // if fopen returns a null pointer, we can't read the file for whatever reason
                // could be permissions, could be file DNE, it doesn't matter to us. We can't use it.
                if (readCachefp != NULL && !fileIsOlderThan(md5path, cacheage))
                {
                    // CACHE HIT!
                    printf("    Cache Hit!\n");
                    putFileInBuffer(responseBuffer, RESPONSE_BUFFER_SIZE, readCachefp);
                    responseSize = getFileSize(readCachefp);
                    fclose(readCachefp);
                }
                else
                {
                    // we missed cache. Either DNE or was too old
                    // Forward the request on to the origin web server
                    printf("    Cache Miss! Forwarding on to web server...\n");
                    http_forward(connfd, responseBuffer, &responseSize, requestMethod, requestPath, httpVersion);

                    // Now that we got a copy, we can cache these files for future use
                    FILE *writeCachefp = fopen(md5path, "wb");
                    printf("    Cached response to %s\n", md5path);
                    putBufferInFile(responseBuffer, responseSize, writeCachefp);
                    fclose(writeCachefp);
                    if (readCachefp != NULL)
                    {
                        fclose(readCachefp);
                    }
                }

                free(md5path);
            }
            else
            {
                // requestIsValid builds our response for us if it's invalid. We can fall out of
                // this and just return to client
            }

            // send response back to client
            sendResponse(connfd, responseBuffer, responseSize);
            // close socket when done
            close(connfd);

            // free memory
            free(responseBuffer);
            break; // break out of the infinite loop and exit
        }
    }
}