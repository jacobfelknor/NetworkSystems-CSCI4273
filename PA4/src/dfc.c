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

    int socks[] = {0, 0, 0, 0};
    char *cmd;
    char *path;
    char *filename;
    char buffer[BUFFER_SIZE];

    /* check command line arguments */
    if (argc < 2)
    {
        fprintf(stderr, "usage: %s <command> [filename] ... [filename]\n", argv[0]);
        exit(0);
    }

    cmd = argv[1];
    // TODO: handle multiple files
    path = argv[2];
    filename = strrchr(path, '/');
    if (filename == NULL)
    {
        filename = path;
    }

    // TODO: read in dfc.conf, set up connections to servers
    char *server = "localhost";
    int ports[] = {8000, 8001, 8002, 8003};
    socks[0] = get_socket(server, ports[0]);
    socks[1] = get_socket(server, ports[1]);
    socks[2] = get_socket(server, ports[2]);
    socks[3] = get_socket(server, ports[3]);

    if (strcmp(cmd, "put") == 0)
    {
        // put some file to servers
        FILE *fp = fopen(path, "rb");
        if (fp != NULL)
        {
            long fsize = putFileInBuffer(buffer, BUFFER_SIZE, fp);
            // split file into 4 parts
            long chunkSize = fsize / 4;
            long lastChunkSize = fsize - 3 * chunkSize;
            char *chunk1 = buffer;
            char *chunk2 = chunk1 + chunkSize;
            char *chunk3 = chunk2 + chunkSize;
            char *chunk4 = chunk3 + chunkSize;

            // chunk 1 & 2 go to server 1
            sendChunk(socks[0], filename, 1, chunk1, chunkSize);
            sendChunk(socks[0], filename, 2, chunk2, chunkSize);

            // chunk 2 & 3 go to server 2
            sendChunk(socks[1], filename, 2, chunk2, chunkSize);
            sendChunk(socks[1], filename, 3, chunk3, chunkSize);

            // chunk 3 & 4 go to server 3
            sendChunk(socks[2], filename, 3, chunk3, chunkSize);
            sendChunk(socks[2], filename, 4, chunk4, lastChunkSize);

            // chunk 4 & 1 go to server 4
            sendChunk(socks[3], filename, 4, chunk4, lastChunkSize);
            sendChunk(socks[3], filename, 1, chunk1, chunkSize);
        }
        else
        {
            error("File could not be opened. DNE or insufficient permissions");
        }
    }

    close(socks[0]);
    close(socks[1]);
    close(socks[2]);
    close(socks[3]);
}