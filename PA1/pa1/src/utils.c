#include "../include/utils.h"
#include "../include/constants.h"

void send_msg(int sockfd, char *buf, struct sockaddr_in addr)
{
    /* 
     * sendto: echo the input back to the client 
     */
    int len;
    int n;
    len = strlen(buf);

    n = sendto(sockfd, buf, strlen(buf), 0,
               (struct sockaddr *)&addr, sizeof(addr));
    if (n < 0)
        error("ERROR in sendto");
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
