#include "../include/utils.h"

extern int errno;

/*
 * error - wrapper for perror
 */
void error(char *msg)
{
    perror(msg);
    exit(1);
}

long getFileSize(FILE *f)
{
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET); /* same as rewind(f); */
    return fsize;
}

void putFileInBuffer(char *buf, int bufsize, FILE *f)
{
    // clear buffer first
    bzero(buf, bufsize);
    long fsize = getFileSize(f);

    if (fsize > bufsize)
    {
        error("Buffer too small to hold this file");
    }

    // fread returns number of elements read.
    // Here, I'm telling it each element is fsize long
    // meaning, on success fread returns 1
    if (fread(buf, fsize, 1, f) != 1)
    {
        error("error on fread..\n");
    }
}

void putBufferInFile(char *buf, int bufsize, FILE *f)
{
    if (f)
    {
        fwrite(buf, bufsize, 1, f);
    }
    else
    {
        error("File pointer error");
    }
}

// send the response
void sendRequest(int connfd, char *responseBuffer, long responseSize)
{
    // double check responseSize fits in buffer
    if (responseSize > RESPONSE_BUFFER_SIZE)
    {
        error("response would not fit in buffer");
    }
    // keep track of how many bytes we've sent so far
    long bytesSent = 0;
    while (bytesSent < responseSize) // only exit loop once we've sent all the bytes
    {
        // how many were sent on this iteration?
        // want to send bytes starting at offset bytesSent, and send the difference between the total and sent so far
        long n = write(connfd, responseBuffer + bytesSent, responseSize - bytesSent);
        if (n < 0)
        {
            // error writing to socket. Exit
            error("error on write");
        }
        else
        {
            // we sent n bytes. Increment our counter
            bytesSent += n;
            printf("Sent %ld/%ld bytes\n", bytesSent, responseSize);
        }
    }
}