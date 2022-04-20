#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "../include/socket.h"

extern int errno;

/*
 * error - wrapper for perror
 */
void error(char *msg)
{
    perror(msg);
    exit(1);
}

int get_socket(char *hostname, int port)
{
    int sockfd;                    /* socket */
    struct sockaddr_in serveraddr; /* server's addr */
    struct hostent *host;          /* host info */
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
    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
               (const void *)&optval, sizeof(int));

    // get hostname
    /* gethostbyname: get the server's DNS entry */
    host = gethostbyname(hostname);
    if (host == NULL)
    {
        error("no such host");
    }

    /* build the server's Internet address */
    bzero((char *)&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)host->h_addr,
          (char *)&serveraddr.sin_addr.s_addr, host->h_length);
    serveraddr.sin_port = htons(port);

    // connect
    if (connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(struct sockaddr_in)) == -1)
    {
        error("Error on connect");
    }

    return sockfd;
}

void writeToSocket(int sockfd, char *buffer, int size)
{
    // write to a socket, checking the return value
    int bytesWritten = 0;
    while (bytesWritten < size)
    {
        bytesWritten += write(sockfd, buffer + bytesWritten, size - bytesWritten);
    }
}

void readFromSocket(int sockfd, char *buffer, int size)
{
    // this function assumes the server will close the connection once done sending bytes
    int bytesRead = 0;
    int chunk = 1;
    while (chunk > 0)
    {
        chunk = read(sockfd, buffer + bytesRead, size - bytesRead);
        bytesRead += chunk;
        if (bytesRead > size)
        {
            error("Buffer too small to read from socket");
        }
    }
}

// send a chunk of a file to a server
void sendChunk(int sockfd, char *filename, int id, char *chunk, int chunkSize)
{
    int MAX_LEN = 1000;
    char serverPutCMD[MAX_LEN];
    bzero(serverPutCMD, MAX_LEN);

    // chunk 1 & 2 go to server 1
    snprintf(serverPutCMD, MAX_LEN, "PUT %s.%d %d\r\n", filename, id, chunkSize);
    writeToSocket(sockfd, serverPutCMD, strlen(serverPutCMD));
    writeToSocket(sockfd, chunk, chunkSize);
}