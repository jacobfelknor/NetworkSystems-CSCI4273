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
    else if (strcmp(cmd, "get") == 0)
    {
        // get files from server
        // send a request GET filename\r\n
        // sever will either respond with OK filename n\r\n<data>
        // or will respond NOT FOUND\r\n or will timeout because its down.

        // client stores each response from server in a buffer labeled for each chunk
        // keep track if we have gotten a successful response and filled buffer for a certain chunk

        // at the end, write buffers 1-4 out in order to a file
        // bool chunk1Found = false;
        // bool chunk2Found = false;
        // bool chunk3Found = false;
        // bool chunk4Found = false;
        char *chunk1 = (char *)malloc(BUFFER_SIZE);
        char *chunk2 = (char *)malloc(BUFFER_SIZE);
        char *chunk3 = (char *)malloc(BUFFER_SIZE);
        char *chunk4 = (char *)malloc(BUFFER_SIZE);
        bzero(chunk1, BUFFER_SIZE);
        bzero(chunk2, BUFFER_SIZE);
        bzero(chunk3, BUFFER_SIZE);
        bzero(chunk4, BUFFER_SIZE);
        char *chunks[] = {chunk1, chunk2, chunk3, chunk4};
        bool chunkFound[] = {false, false, false, false};
        bool fileComplete = true;
        int chunkSizes[] = {0, 0, 0, 0};

        // ask each server for each chunk, see what they return
        int MAX_LEN = 100;
        char *serverCMD = (char *)malloc(MAX_LEN);
        char *serverCMDmemory = serverCMD;
        char filenamecp[MAX_LEN];
        bzero(filenamecp, MAX_LEN);
        strcpy(filenamecp, filename);
        for (int j = 0; j < 4; j++)
        {

            for (int i = 1; i < 5; i++)
            {
                bzero(serverCMD, MAX_LEN);
                snprintf(serverCMD, MAX_LEN, "GET %s.%d\r\n", filenamecp, i);
                writeToSocket(socks[j], serverCMD, strlen(serverCMD));
                // TODO: I'm locking the chunksize returned at 250 because I know this file. GENERALIZE!!!!!!!!
                // also, write to a temp buffer first. If the temp bytes are still null after
                // reading, we know the server didn't have that file and so we shouldn't copy it to the chunk buffer
                bzero(serverCMD, MAX_LEN);
                readLineFromSocket(socks[j], serverCMD, MAX_LEN);
                char tempFileName[MAX_LEN]; // just need to give parseRequest something. We really just need the chunkSize
                int chunkSize = 0;
                parseRequest(&serverCMD, cmd, tempFileName, &chunkSize);
                if (strcmp(cmd, "OK") == 0)
                {
                    chunkFound[i - 1] = true;
                    chunkSizes[i - 1] = chunkSize;
                    readFromSocket(socks[j], chunks[i - 1], chunkSizes[i - 1]);
                }
                // printf("%s\n", chunks[i - 1]);
                // close and reopen connection to make a new request
                serverCMD = serverCMDmemory;
                bzero(serverCMDmemory, MAX_LEN);
                close(socks[j]);
                socks[j] = get_socket(server, ports[j]);
            }
        }

        for (int i = 0; i < 4; i++)
        {
            if (!chunkFound[i])
            {
                fileComplete = false;
            }
        }
        if (fileComplete)
        {
            // now write the file to disk
            // TODO: instead of blindly writing, check if chunk is all NULL, meaning the file is incomplete
            FILE *fp = fopen("recieved_file", "wb");
            fwrite(chunk1, chunkSizes[0], 1, fp);
            fwrite(chunk2, chunkSizes[1], 1, fp);
            fwrite(chunk3, chunkSizes[2], 1, fp);
            fwrite(chunk4, chunkSizes[3], 1, fp);
            fclose(fp);
        }
        else
        {
            fprintf(stderr, "%s is incomplete\n", filenamecp);
            exit(0);
        }

        // free memory
        free(chunk1);
        free(chunk2);
        free(chunk3);
        free(chunk4);
        free(serverCMDmemory);
    }
    else if (strcmp(cmd, "list") == 0)
    {
        // list files from server
    }
    else
    {
        fprintf(stderr, "Invalid command '%s'. Must be 'put', 'get', or 'list'\n", cmd);
        exit(0);
    }

    close(socks[0]);
    close(socks[1]);
    close(socks[2]);
    close(socks[3]);
}