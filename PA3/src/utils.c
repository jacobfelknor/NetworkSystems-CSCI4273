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

// https://stackoverflow.com/a/4553053
int isDirectory(const char *path)
{
    struct stat statbuf;
    if (stat(path, &statbuf) != 0)
        return 0;
    return S_ISDIR(statbuf.st_mode);
}

// https://stackoverflow.com/a/2336245
void r_mkdir(const char *dir)
{
    char tmp[PATH_MAX];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", dir);
    len = strlen(tmp);
    if (tmp[len - 1] == '/')
        tmp[len - 1] = 0;
    for (p = tmp + 1; *p; p++)
        if (*p == '/')
        {
            *p = 0;
            mkdir(tmp, S_IRWXU);
            *p = '/';
        }
    mkdir(tmp, S_IRWXU);
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

// put content in the response buff. Return total length
long appendContent(char *responseBuffer, char *fileBuffer, long fileSize)
{
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

    return currlen + fileSize;
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

// send the client back a copy of the file specified at path
void reply(int connfd, char *responseBuffer, long *responseSize, char *requestPath, char *requestMethod, char *httpVersion)
{
    // create a buffer to store the file
    char *fileBuffer = (char *)malloc(RESPONSE_FILE_BUFFER);
    bzero(fileBuffer, RESPONSE_FILE_BUFFER);
    long fileSize;
    bool pathMallocd = false;
    // open the given filepath and put into the buffer
    FILE *fp;
    if (!isDirectory(requestPath))
    {
        // not a directory, just attempt to open the file
        fp = fopen(requestPath, "r");
    }
    else
    {
        // this is a directory. Need to look for an index.html here
        int pathLength = strlen(requestPath);
        char *indexHTML;
        if (requestPath[pathLength - 1] == '/') // pathLength - 1 because of null terminator
        {
            // did the request path end in a slash?
            indexHTML = "index.html";
        }
        else
        {
            indexHTML = "/index.html";
        }

        char *newRequestPath = strConcat(requestPath, indexHTML);

        printf("Requested a directory, looking now at %s\n", newRequestPath);
        requestPath = newRequestPath;
        pathMallocd = true;
        fp = fopen(newRequestPath, "r");
    }

    if (!requestIsValid(responseBuffer, requestPath, requestMethod, httpVersion, responseSize))
    {
        // request wasn't valid, return out
        return;
    }
    else if (fp == NULL)
    {
        if (errno == ENOENT)
        {
            // ENOENT: No such file or directory
            *responseSize = buildResponse(responseBuffer, httpVersion, "404 Not Found", "text/html", 0);
        }
        else if (errno == EACCES)
        {
            // EACCES: Insufficient permissions to open the file
            *responseSize = buildResponse(responseBuffer, httpVersion, "403 Forbidden", "text/html", 0);
        }
    }
    else
    {
        fileSize = getFileSize(fp);
        // instead of putting file here, put it directly in response buffer
        // putFileInBuffer(responseBuffer + strlen(responseBuffer), RESPONSE_FILE_BUFFER, fp)
        putFileInBuffer(fileBuffer, RESPONSE_FILE_BUFFER, fp);

        // start to build response
        char *statusCode = "200 OK";
        char *contentType = getContentType(requestPath);
        long contentLength = fileSize;

        buildResponse(responseBuffer, httpVersion, statusCode, contentType, contentLength);
        *responseSize = appendContent(responseBuffer, fileBuffer, fileSize);
        fclose(fp);
    }

    // free our memory used
    free(fileBuffer);
    if (pathMallocd)
    {
        free(requestPath);
    }
}
