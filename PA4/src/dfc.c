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

    int sockfd, port;
    // socklen_t serverlen;
    // struct sockaddr_in serveraddr;
    // struct hostent *server;
    char *hostname;
    char *cmd;
    char responseBuffer[RESPONSE_BUFFER_SIZE];

    /* check command line arguments */
    if (argc < 2)
    {
        fprintf(stderr, "usage: %s <command> [filename] ... [filename]\n", argv[0]);
        exit(0);
    }

    cmd = argv[1];

    hostname = "localhost";
    port = 8000;

    sockfd = get_socket(hostname, port);

    // write the cmd to the dfs
    writeToSocket(sockfd, cmd, strlen(cmd));

    // read the reply from the dfs
    readFromSocket(sockfd, responseBuffer, RESPONSE_BUFFER_SIZE);

    // print the reply
    printf("%s\n", responseBuffer);

    close(sockfd);
}