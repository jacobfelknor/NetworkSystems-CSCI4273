#include "../include/utils.h"
#include "../include/str_utils.h"

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

// copy the web request string into the given buffer
void request2buffer(int connfd, char *buf, int bufsize)
{
    // clear any existing data
    bzero(buf, bufsize);
    // store request in our buffer
    // read is a blocking call until bytes are sent to buffer
    // Assume we got everything in one read, will throw 400 if malformed
    if (read(connfd, buf, bufsize) < 0)
    {
        error("error parsing request from socket..\n");
    }
}

// build a response with the specified information
long buildResponse(char *responseBuffer, char *httpVersion, char *statusCode, char *contentType, long contentLength)
{
    if (contentType != NULL && contentLength != -1)
    {

        snprintf(responseBuffer, RESPONSE_BUFFER_SIZE,
                 "%s %s\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n",
                 httpVersion, statusCode, contentType, contentLength);
    }
    else
    {
        snprintf(responseBuffer, RESPONSE_BUFFER_SIZE,
                 "%s %s\r\n\r\n",
                 httpVersion, statusCode);
    }
    // so far, the response is just a string. We can use strlen
    return strlen(responseBuffer);
}

// send the response
void sendResponse(int connfd, char *responseBuffer, long responseSize)
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

// validate the 3 inputs COULD be valid...
bool validateRequestParams(char *requestPath, char *requestMethod, char *httpVersion)
{
    if (!(strlen(requestPath) && strlen(requestMethod) && strlen(httpVersion)))
    {
        return false;
    }

    if (!startsWith(httpVersion, "HTTP/"))
    {
        return false;
    }

    return true;
}

// send some common response codes
bool requestIsValid(char *responseBuffer, char *requestPath, char *requestMethod, char *httpVersion, long *responseSize)
{
    if (!validateRequestParams(requestPath, requestMethod, httpVersion))
    {
        *responseSize = buildResponse(responseBuffer, "HTTP/1.1", "400 Bad Request", "text/html", 0);
        return false;
    }
    else if (!(strcmp(httpVersion, "HTTP/1.1") == 0 || strcmp(httpVersion, "HTTP/1.0") == 0))
    {
        *responseSize = buildResponse(responseBuffer, httpVersion, "505 HTTP Version Not Supported", "text/html", 0);
        return false;
    }
    else if (strcmp(requestMethod, "GET") != 0)
    {
        *responseSize = buildResponse(responseBuffer, httpVersion, "405 Method Not Allowed", "text/html", 0);
        return false;
    }

    return true;
}