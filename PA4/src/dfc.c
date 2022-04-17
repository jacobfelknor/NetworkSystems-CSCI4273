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
    char *filename;
    char responseBuffer[RESPONSE_BUFFER_SIZE];

    /* check command line arguments */
    if (argc < 2)
    {
        fprintf(stderr, "usage: %s <command> [filename] ... [filename]\n", argv[0]);
        exit(0);
    }

    cmd = argv[1];
    // TODO: handle multiple files
    filename = argv[2];

    hostname = "localhost";
    port = 8000;

    sockfd = get_socket(hostname, port);

    if (strcmp(cmd, "put") == 0)
    {
        // put some file to servers
        FILE *fp = fopen(filename, "rb");
        if (fp != NULL)
        {
            long fsize = putFileInBuffer(responseBuffer, RESPONSE_BUFFER_SIZE, fp);
            // split file into 4 parts
            long chunkSize = fsize / 4;
            long lastChunkSize = fsize - 3 * chunkSize;
            char *chunk1 = responseBuffer;
            char *chunk2 = chunk1 + chunkSize;
            char *chunk3 = chunk2 + chunkSize;
            char *chunk4 = chunk3 + chunkSize;

            writeToSocket(sockfd, chunk1, chunkSize);
            writeToSocket(sockfd, chunk2, chunkSize);
            writeToSocket(sockfd, chunk3, chunkSize);
            writeToSocket(sockfd, chunk4, lastChunkSize);
        }
        else
        {
            error("File could not be opened. DNE or insufficient permissions");
        }
    }
    // write the cmd to the dfs
    // writeToSocket(sockfd, cmd, strlen(cmd));

    // read the reply from the dfs
    // readFromSocket(sockfd, responseBuffer, RESPONSE_BUFFER_SIZE);

    // print the reply
    // printf("%s\n", responseBuffer);

    close(sockfd);
}