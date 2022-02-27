#include "../include/utils.h"

/*
 * error - wrapper for perror
 */
void error(char *msg)
{
    perror(msg);
    exit(0);
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

    fread(buf, fsize, 1, f);
    fclose(f);

    // string[fsize] = 0;
}

// copy the web request string into the given buffer
void request2buffer(int connfd, char *buf, int bufsize)
{
    // clear any existing data
    bzero(buf, bufsize);
    // store request in our buffer
    read(connfd, buf, bufsize);
}

// send the client back a copy of the file specified at path
void reply(int connfd, char *path)
{
    // create a buffer to store the file
    char *fileBuffer = (char *)malloc(RESPONSE_FILE_BUFFER);
    char *responseBuffer = (char *)malloc(RESPONSE_BUFFER_SIZE);
    bzero(fileBuffer, RESPONSE_FILE_BUFFER);
    bzero(responseBuffer, RESPONSE_BUFFER_SIZE);
    int fileSize;

    // open the given filepath and put into the buffer
    FILE *fp;
    fp = fopen("/home/jacob2/Development/NetworkSystems-CSCI4273/PA2/www/simple.html", "r");
    if (fp == NULL)
    {
        error("SHOULD RETURN A 404 HERE!!");
    }
    else
    {
        fileSize = getFileSize(fp);
    }
    putFileInBuffer(fileBuffer, RESPONSE_FILE_BUFFER, fp);

    // start to build response
    char *httpVersion = "HTTP/1.1";
    char *statusCode = "200 OK";
    char *contentType = "text/html";
    long contentLength = fileSize;

    snprintf(responseBuffer, RESPONSE_BUFFER_SIZE,
             "%s %s\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n",
             httpVersion, statusCode, contentType, contentLength);

    // get position in response buffer to put file
    int currlen = strlen(responseBuffer);
    // validate we can fit file buffer in response buffer
    if (currlen + fileSize > RESPONSE_BUFFER_SIZE)
    {
        error("response buffer is too small");
    }
    else
    {
        // copy file into responseBuffer
        for (int i = 0; i < fileSize; i++)
        {
            responseBuffer[i + currlen] = fileBuffer[i];
        }
    }

    // write buffer to the client
    write(connfd, responseBuffer, RESPONSE_BUFFER_SIZE);
    fclose(fp);
}