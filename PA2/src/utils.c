#include "../include/utils.h"

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

char *getFileExtension(char *path)
{
    // adapted from https://stackoverflow.com/a/5309514
    char *ext;
    ext = strrchr(path, '.');
    if (!ext)
    {
        return "";
    }
    else
    {
        return ext;
    }
}

char *getContentType(char *path)
{
    char *ext = getFileExtension(path);

    if (strcmp(ext, ".html") == 0)
    {
        return "text/html";
    }
    else if (strcmp(ext, ".txt") == 0)
    {
        return "text/plain";
    }
    else if (strcmp(ext, ".png") == 0)
    {
        return "image/png";
    }
    else if (strcmp(ext, ".gif") == 0)
    {
        return "image/gif";
    }
    else if (strcmp(ext, ".jpg") == 0)
    {
        return "image/jpg";
    }
    else if (strcmp(ext, ".css") == 0)
    {
        return "text/css";
    }
    else if (strcmp(ext, ".js") == 0)
    {
        return "application/javascript";
    }
    else if (strcmp(ext, ".ico") == 0)
    {
        return "image/ico";
    }
    else
    {
        // let the browser guess
        return "";
    }
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
}

// copy the web request string into the given buffer
void request2buffer(int connfd, char *buf, int bufsize)
{
    // clear any existing data
    bzero(buf, bufsize);
    // store request in our buffer
    read(connfd, buf, bufsize);
}

// build a response with the specified information
void buildResponse(char *responseBuffer, char *httpVersion, char *statusCode, char *contentType, long contentLength)
{
    snprintf(responseBuffer, RESPONSE_BUFFER_SIZE,
             "%s %s\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n",
             httpVersion, statusCode, contentType, contentLength);
}

// send the response
void sendResponse(int connfd, char *responseBuffer)
{
    write(connfd, responseBuffer, RESPONSE_BUFFER_SIZE);
}

// send the client back a copy of the file specified at path
void reply(int connfd, char *path, char *httpVersion)
{
    // create a buffer to store the file
    char *fileBuffer = (char *)malloc(RESPONSE_FILE_BUFFER);
    char *responseBuffer = (char *)malloc(RESPONSE_BUFFER_SIZE);
    bzero(fileBuffer, RESPONSE_FILE_BUFFER);
    bzero(responseBuffer, RESPONSE_BUFFER_SIZE);
    int fileSize;

    // open the given filepath and put into the buffer
    FILE *fp;
    fp = fopen(path, "r");
    if (fp == NULL)
    {
        buildResponse(responseBuffer, httpVersion, "404 Not Found", "", 0);
    }
    else
    {
        fileSize = getFileSize(fp);
        putFileInBuffer(fileBuffer, RESPONSE_FILE_BUFFER, fp);

        // start to build response
        char *statusCode = "200 OK";
        char *contentType = getContentType(path);
        long contentLength = fileSize;

        buildResponse(responseBuffer, httpVersion, statusCode, contentType, contentLength);

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
        fclose(fp);
    }

    // write buffer to the client
    sendResponse(connfd, responseBuffer);
    free(fileBuffer);
    free(responseBuffer);
}