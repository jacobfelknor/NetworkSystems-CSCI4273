#include "../include/utils.h"

long getFileSize(FILE *f)
{
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET); /* same as rewind(f); */
    return fsize;
}

long putFileInBuffer(char *buf, int bufsize, FILE *f)
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

    return fsize;
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

// https://stackoverflow.com/a/20300544
// get a line from a buffer
// return place in buffer where I left off
char *sgets(char *s, int n, char **strp)
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
    return *strp;
}

// copy the web request string into the given buffer
void request2buffer(int connfd, char *buf, int bufsize)
{
    // clear any existing data
    bzero(buf, bufsize);
    // store request in our buffer
    readFromSocket(connfd, buf, bufsize);
}

// get my custom request
void parseRequest(char **request, char *cmd, char *filename, int *chunkSize)
{
    // For these 3 pieces of information, only the first line is required.
    int lineSize = 300;
    char firstLine[lineSize]; // should be plenty long
    // need a char version of chunkSize too
    char chunkSizeChar[100]; // should be plenty long

    // get first line, set request to location where file contents start
    *request = sgets(firstLine, lineSize, request);

    char *parts[] = {cmd, filename, chunkSizeChar};
    int part = 0;
    int j = 0;
    // split string into parts by space
    for (int i = 0; i <= strlen(firstLine); i++)
    {
        if (part > 2)
        {
            // reached the end of available parts. If its not done, we can't parse this http request
            break;
        }
        // adapted from includehelp.com/c-programs/c-program-to-split-string-by-space-into-words.aspx
        if (firstLine[i] == ' ' || firstLine[i] == '\0')
        {
            parts[part][j] = '\0';
            part++; // for next part
            j = 0;  // for next part, init index to 0
        }
        else
        {
            parts[part][j] = firstLine[i];
            j++;
        }
    }

    // at this point, replace the newline if it exists from the httpVersion
    chunkSizeChar[strcspn(chunkSizeChar, "\n")] = 0;
    chunkSizeChar[strcspn(chunkSizeChar, "\r")] = 0;
    *chunkSize = atoi(chunkSizeChar);
}

// https://stackoverflow.com/a/8465083
char *pathConcat(char *path1, char *path2)
{
    char *result = malloc(strlen(path1) + strlen(path2) + 2); // +1 for the null-terminator, +1 for the possible "/"
    // in real code you would check for errors in malloc here
    strcpy(result, path1);
    if (path2[0] == '.')
    {
        // assume path given is like ./path/to/file
        path2 += 1;
    }
    if (result[strlen(path1)] != '/' && path2[0] != '/')
    {
        strcat(result, "/");
    }
    strcat(result, path2);
    return result;
}

void serverPutFile(char *request, char *cmd, char *dir, char *filename, int chunkSize)
{
    // make the dir if needed
    mkdir(dir, 0775);
    // do the first chunk. The main function has already parsed the first request
    // we can start right away
    char *path = pathConcat(dir, filename);
    printf("cmd: %s, path: %s, chunkSize: %d\n", cmd, path, chunkSize);
    FILE *fp;
    fp = fopen(path, "wb");
    fwrite(request, chunkSize, 1, fp);
    fclose(fp);

    // do the second chunk. The above will leave us with the second request
    // ready to read and use. Should always be PUT...
    // this is assuming 2 chunks sent in pairs, as write up suggests. Is not flexible
    request += chunkSize;
    parseRequest(&request, cmd, filename, &chunkSize);
    path = pathConcat(dir, filename);
    printf("cmd: %s, path: %s, chunkSize: %d\n", cmd, path, chunkSize);
    fp = fopen(path, "wb");
    fwrite(request, chunkSize, 1, fp);
    fclose(fp);
    free(path);
}

void clientPutFile(char *path, char *buffer, int *socks, char *filename)
{
    FILE *fp = fopen(path, "rb");
    if (fp != NULL)
    {
        long fsize = putFileInBuffer(buffer, BUFFER_SIZE, fp);
        // split file into 4 parts
        long chunkSize = fsize / 4;
        long lastChunkSize = fsize - 3 * chunkSize;
        char *chunk1 = buffer;
        char *chunk2 = chunk1 + chunkSize;
        char *chunk3 = chunk2 + chunkSize;
        char *chunk4 = chunk3 + chunkSize;

        // chunk 1 & 2 go to server 1
        sendChunk(socks[0], filename, 1, chunk1, chunkSize);
        sendChunk(socks[0], filename, 2, chunk2, chunkSize);

        // chunk 2 & 3 go to server 2
        sendChunk(socks[1], filename, 2, chunk2, chunkSize);
        sendChunk(socks[1], filename, 3, chunk3, chunkSize);

        // chunk 3 & 4 go to server 3
        sendChunk(socks[2], filename, 3, chunk3, chunkSize);
        sendChunk(socks[2], filename, 4, chunk4, lastChunkSize);

        // chunk 4 & 1 go to server 4
        sendChunk(socks[3], filename, 4, chunk4, lastChunkSize);
        sendChunk(socks[3], filename, 1, chunk1, chunkSize);
    }
    else
    {
        error("File could not be opened. DNE or insufficient permissions");
    }
}