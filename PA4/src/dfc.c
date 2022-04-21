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
    filename = strrchr(path, '/'); // want the base filename when uploading to the server
    if (filename == NULL)
    {
        // path did not include '/'. The path is the filename (assuming the cwd)
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
        clientPutFile(path, buffer, socks, filename);
    }

    close(socks[0]);
    close(socks[1]);
    close(socks[2]);
    close(socks[3]);
}