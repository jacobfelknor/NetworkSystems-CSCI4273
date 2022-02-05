#include "../include/utils.h"
#include "../include/constants.h"

/* 
 * error - wrapper for perror
 */
void error(char *msg)
{
    perror(msg);
    exit(0);
}

void send_msg(int sockfd, char *buf, struct sockaddr_in addr)
{
    /* 
     * sendto: send a message to another host
     */
    int len;
    int n;
    len = strlen(buf);

    n = sendto(sockfd, buf, strlen(buf), 0,
               (struct sockaddr *)&addr, sizeof(addr));
    if (n < 0)
        error("ERROR in sendto");
}

void get_msg_timeout(int sockfd, char *buf, struct sockaddr_in addr)
{
    // get a message from a host, with a timeout
    // timeout code from http://alumni.cs.ucr.edu/~jiayu/network/lab8.htm
    int n;
    fd_set readfds, masterfds;
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    FD_ZERO(&masterfds);
    FD_SET(sockfd, &masterfds);
    memcpy(&readfds, &masterfds, sizeof(fd_set));

    if (select(sockfd + 1, &readfds, NULL, NULL, &timeout) < 0)
    {
        error("Error on select");
    }

    if (FD_ISSET(sockfd, &readfds))
    {
        // Okay to read from socket
        socklen_t addrlen = sizeof(addr);
        n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&addr, &addrlen);
        if (n < 0)
            error("ERROR in recvfrom");
    }
    else
    {
        // The socket timed out. Print to stderr
        error("Socket timed out. Exiting");
    }
}

void putFileInBuffer(char *buf, FILE *f)
{
    // clear buffer first
    bzero(buf, BUFSIZE);
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET); /* same as rewind(f); */

    // char *string = malloc(fsize + 1);
    // if (fsize > sizeof buf - 1)
    // {
    //   // only fill buffer
    //   fsize = sizeof buf - 1;
    // }
    fread(buf, fsize, 1, f);
    fclose(f);

    // string[fsize] = 0;
}

void captureCmdOutput(char *cmd, char *buf)
{
    FILE *fp;
    int status;
    char path[PATH_MAX];

    fp = popen(cmd, "r");
    if (fp == NULL)
        /* Handle error */;
    // clear buffer first, then store contents of ls
    putFileInBuffer(buf, fp);
}

// https://stackoverflow.com/a/20300544
// get a line from a buffer
char *sgets(char *s, int n, const char **strp)
{
    if (**strp == '\0')
        return NULL;
    int i;
    for (i = 0; i < n - 1; ++i, ++(*strp))
    {
        s[i] = **strp;
        if (**strp == '\0')
            break;
        if (**strp == '\n')
        {
            s[i + 1] = '\0';
            ++(*strp);
            break;
        }
    }
    if (i == n - 1)
        s[i] = '\0';
    return s;
}
