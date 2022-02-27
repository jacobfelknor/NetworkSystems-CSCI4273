#include "../include/utils.h"

/*
 * error - wrapper for perror
 */
void error(char *msg)
{
    perror(msg);
    exit(0);
}

// copy the web request string into the given buffer
void request2buffer(int connfd, char *buf, int bufsize)
{
    // clear any existing data
    bzero(buf, bufsize);
    // store request in our buffer
    read(connfd, buf, bufsize);
}

// Reply to a client request
void reply(int connfd)
{
    char buff[BUFSIZ];
    int n;
    // infinite loop for chat
    for (;;)
    {
        bzero(buff, BUFSIZ);

        // read the message from client and copy it in buffer
        read(connfd, buff, sizeof(buff));
        // print buffer which contains the client contents
        printf("From client: %s\t To client : ", buff);
        bzero(buff, BUFSIZ);
        n = 0;
        // copy server message in the buffer
        while ((buff[n++] = getchar()) != '\n')
            ;

        // and send that buffer to client
        write(connfd, buff, sizeof(buff));

        // if msg contains "Exit" then server exit and chat ended.
        if (strncmp("exit", buff, 4) == 0)
        {
            printf("Server Exit...\n");
            break;
        }
    }
}
