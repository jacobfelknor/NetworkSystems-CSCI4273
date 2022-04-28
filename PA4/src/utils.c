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
    // readFromSocket(connfd, buf, bufsize);
    readLineFromSocket(connfd, buf, bufsize);
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
    sgets(firstLine, lineSize, request);

    char *parts[] = {cmd, filename, chunkSizeChar};
    int part = 0;
    int j = 0;
    // split string into parts by space
    for (int i = 0; i <= strlen(firstLine); i++)
    {
        if (part > 2)
        {
            // reached the end of available parts. If its not done, we can't parse this request
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

    // at this point, replace the newline if it exists
    chunkSizeChar[strcspn(chunkSizeChar, "\n")] = 0;
    chunkSizeChar[strcspn(chunkSizeChar, "\r")] = 0;
    filename[strcspn(filename, "\n")] = 0;
    filename[strcspn(filename, "\r")] = 0;
    cmd[strcspn(cmd, "\n")] = 0;
    cmd[strcspn(cmd, "\r")] = 0;
    *chunkSize = atoi(chunkSizeChar);
}

// adapted from https://stackoverflow.com/a/726214
void splitHost(char *line, char *host, int *port)
{
    // we only care about the last item in this line. iterate backwards until a space
    int lineLength = strlen(line);
    char *ptr = line + lineLength;
    for (int i = 0; i < lineLength; i++)
    {
        if (*ptr == ' ')
        {
            ptr += 1;
            break;
        }
        ptr -= 1;
    }
    // extract port
    sscanf(ptr, "%*[^:]:%d", port);
    // truncate at :
    ptr[strcspn(ptr, ":")] = 0;
    // copy the host into given spot
    for (int i = 0; i < strlen(ptr); i++)
    {
        host[i] = *(ptr + i);
    }
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

int hashMod4(char *filename)
{
    unsigned char digest[16];
    char *digeststr = (char *)malloc(33); // 32 chars plus null term
    bzero(digeststr, 33);
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, filename, strlen(filename));
    MD5_Final(digest, &ctx);
    for (int i = 0, j = 0; i < 16; i++, j += 2)
    {
        sprintf(digeststr + j, "%02x", digest[i]);
    }
    // computes based on the last 16 bytes of the hash
    // https://stackoverflow.com/a/11180162
    size_t len = strlen(digeststr);
    size_t offset = len < 16 ? 0 : len - 16;
    unsigned long long hash_tail = strtoull((digeststr + offset), NULL, 16);
    free(digeststr);
    return hash_tail % 4;
}

void serverPutFile(int sockfd, char *request, char *cmd, char *dir, char *filename, int chunkSize)
{
    // make the dir if needed
    mkdir(dir, 0775);
    // do the first chunk. The main function has already parsed the first request
    // we can start right away
    // need to read chunkSize bytes from socket now.
    char *chunk = (char *)malloc(chunkSize);
    readFromSocket(sockfd, chunk, chunkSize);
    char *path = pathConcat(dir, filename);
    printf("cmd: %s, path: %s, chunkSize: %d\n", cmd, path, chunkSize);
    FILE *fp;
    fp = fopen(path, "wb");
    fwrite(chunk, chunkSize, 1, fp);
    fclose(fp);

    // do the second chunk.
    // this is assuming 2 chunks sent in pairs, as write up suggests. Is not flexible
    request2buffer(sockfd, request, BUFFER_SIZE);
    parseRequest(&request, cmd, filename, &chunkSize);
    readFromSocket(sockfd, chunk, chunkSize);
    path = pathConcat(dir, filename);
    printf("cmd: %s, path: %s, chunkSize: %d\n", cmd, path, chunkSize);
    fp = fopen(path, "wb");
    fwrite(chunk, chunkSize, 1, fp);
    fclose(fp);
    free(path);
    free(chunk);
}

void clientPutFile(char *path, char *buffer, int *socks, char *filename)
{
    for (int i = 0; i < 4; i++)
    {
        if (socks[i] < 0)
        {
            fprintf(stderr, "%s put failed\n", filename);
            exit(0);
        }
    }
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

        int x = hashMod4(filename);

        // chunk 1 & 2 go to server 1
        sendChunk(socks[(0 + x) % 4], filename, 1, chunk1, chunkSize);
        sendChunk(socks[(0 + x) % 4], filename, 2, chunk2, chunkSize);
        // chunk 2 & 3 g(o+x)%4 to server 2
        sendChunk(socks[(1 + x) % 4], filename, 2, chunk2, chunkSize);
        sendChunk(socks[(1 + x) % 4], filename, 3, chunk3, chunkSize);
        // chunk 3 & 4 g(o+x)%4 to server 3
        sendChunk(socks[(2 + x) % 4], filename, 3, chunk3, chunkSize);
        sendChunk(socks[(2 + x) % 4], filename, 4, chunk4, lastChunkSize);
        // chunk 4 & 1 g(o+x)%4 to server 4
        sendChunk(socks[(3 + x) % 4], filename, 4, chunk4, lastChunkSize);
        sendChunk(socks[(3 + x) % 4], filename, 1, chunk1, chunkSize);
        fclose(fp);
    }
    else
    {
        error("File could not be opened. DNE or insufficient permissions");
    }
}

void clientGetFile(char **servers, int *ports, int *socks, char *filename)
{
    // send a request GET filename\r\n
    // sever will either respond with OK filename n\r\n<data>
    // or will respond NOT FOUND\r\n or will timeout because its down.

    // client stores each response from server in a buffer labeled for each chunk
    // keep track if we have gotten a successful response and filled buffer for a certain chunk

    // at the end, write buffers 1-4 out in order to a file
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
    char *cmdBuffer = (char *)malloc(MAX_LEN);
    char *serverCMDmemory = cmdBuffer;
    char filenamecp[MAX_LEN];
    bzero(filenamecp, MAX_LEN);
    strcpy(filenamecp, filename);
    for (int j = 0; j < 4; j++)
    {

        for (int i = 1; i < 5; i++)
        {
            bzero(cmdBuffer, MAX_LEN);
            snprintf(cmdBuffer, MAX_LEN, "GET %s.%d\r\n", filenamecp, i);
            if (socks[j] != -1) // if the sock was -1, that means the server didn't accept the connection
            {
                // sends the request to the server. i.e GET README.1
                writeToSocket(socks[j], cmdBuffer, strlen(cmdBuffer));
                // also, write to a temp buffer first. If the temp bytes are still null after
                // reading, we know the server didn't have that file and so we shouldn't copy it to the chunk buffer
                bzero(cmdBuffer, MAX_LEN);
                readLineFromSocket(socks[j], cmdBuffer, MAX_LEN);
                char tempFileName[MAX_LEN]; // just need to give parseRequest something. We really just need the chunkSize
                char tempcmd[MAX_LEN];
                int chunkSize = 0;
                parseRequest(&cmdBuffer, tempcmd, tempFileName, &chunkSize);
                if (strcmp(tempcmd, "OK") == 0)
                {
                    // getting OK back from server means it was found!
                    chunkFound[i - 1] = true;
                    chunkSizes[i - 1] = chunkSize;
                    readFromSocket(socks[j], chunks[i - 1], chunkSizes[i - 1]);
                }
                else
                {
                    // this means server returned NOT FOUND. we just skip it, try the next one
                }
                // close and reopen connection to make a new request. required since my server closes the connection
                cmdBuffer = serverCMDmemory;
                bzero(serverCMDmemory, MAX_LEN);
                close(socks[j]);
                socks[j] = get_socket(servers[j], ports[j]);
            }
        }
    }

    // check if the file is complete, meaning we got at least one server to respond to our request for that chunk.
    for (int i = 0; i < 4; i++)
    {
        if (!chunkFound[i])
        {
            fileComplete = false;
        }
    }

    if (fileComplete)
    {
        // now write the file to disk since we have all chunks
        FILE *fp = fopen(filenamecp, "wb");
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

void serverGetFile(int connfd, char *dir, char *filename, char *cmd)
{
    // return the file if it exists, otherwise send back a NOT FOUND
    int chunkSize = 0;
    char *response = (char *)malloc(BUFFER_SIZE);
    char *path = pathConcat(dir, filename);
    FILE *fp = fopen(path, "rb");
    if (fp != NULL)
    {
        // if file found, we can write it to socket
        // if its not found, the server will just fall
        // through and close connection, returning nothing to client
        chunkSize = getFileSize(fp);
        int n = snprintf(response, BUFFER_SIZE, "OK %s %d\r\n", filename, chunkSize);
        long fileSize = putFileInBuffer(response + n, BUFFER_SIZE - n, fp);
        writeToSocket(connfd, response, fileSize + n);
        printf("cmd: %s, path: %s, chunkSize: %d\n", cmd, path, chunkSize);
    }
    else
    {
        printf("NOT FOUND: %s\n", path);
        writeToSocket(connfd, "ERROR NOT FOUND\r\n", strlen("ERROR NOT FOUND\r\n"));
    }
    free(response);
    free(path);
}

void clientList(char **servers, int *ports, int *socks)
{
    // bool chunkFound[] = {false, false, false, false};
    // bool fileComplete = true;
    int j = 0;
    char *cmdBuffer = (char *)malloc(BUFFER_SIZE);
    char *filenames = (char *)malloc(BUFFER_SIZE);
    // char *serverCMDmemory = cmdBuffer;
    char *response = (char *)malloc(BUFFER_SIZE);
    // char *allFiles = (char *)malloc(BUFFER_SIZE);
    // char *responseMemory = response;

    bzero(response, BUFFER_SIZE);
    // bzero(allFiles, BUFFER_SIZE);
    bzero(cmdBuffer, BUFFER_SIZE);

    for (int i = 0; i < 4; i++)
    {

        if (socks[i] > 0)
        {

            writeToSocket(socks[i], "LIST\r\n", strlen("LIST\r\n"));
            readFromSocket(socks[i], response + j, BUFFER_SIZE - j);
            j += strlen(response + j);
        }
    }

    // iterate over servers, ask them to list ls -1
    // combine all answers into a file
    // call sort -u /tmp/dfc_jacobfelknor, this gives unique lines

    // give 1 occurance of each filename
    // sort -u test | sed 's/\.[^.]*$//' | sort -u

    // count # of lines that start with filename in /tmp/dfc_jacobfelknor
    // sort -u test | grep '^filename' | wc -l

    // for each filename in filenames
    //      count # of lines that start with
    //      if # == 4: complete else: incomplete

    // put all filenames in a file
    FILE *fp = fopen("/tmp/dfc_all_filenames", "w");
    putBufferInFile(response, strlen(response), fp);
    fclose(fp);
    fp = fopen("/tmp/dfc_sorted_filenames", "w");
    captureCmdOutput("sort -u /tmp/dfc_all_filenames", filenames, BUFFER_SIZE);
    putBufferInFile(filenames, strlen(filenames), fp);
    fclose(fp);
    // printf("%s\n", filenames);

    // put base filenames in a file
    fp = fopen("/tmp/dfc_base_filenames", "w");
    captureCmdOutput("sort -u /tmp/dfc_sorted_filenames | sed 's/\\.[^.]*$//' | sort -u", filenames, BUFFER_SIZE);
    putBufferInFile(filenames, strlen(filenames), fp);
    // printf("%s\n", filenames);
    fclose(fp);

    // for each file in base filenames, check count in all filenames
    fp = fopen("/tmp/dfc_base_filenames", "r");
    char line[250];
    char cmd[350];
    char countChar[10];
    int count;
    bzero(countChar, 10);
    bzero(cmd, 350);
    while (fgets(line, sizeof(line), fp))
    {
        line[strcspn(line, "\n")] = 0;
        sprintf(cmd, "sort -u /tmp/dfc_sorted_filenames | grep '^%s' | wc -l", line);
        captureCmdOutput(cmd, countChar, 10);
        count = atoi(countChar);
        if (count == 4)
        {
            printf("%s\n", line);
        }
        else if (count < 4)
        {
            printf("%s [incomplete]\n", line);
        }
    }
    fclose(fp);

    // free memory and remove tmp files
    remove("/tmp/dfc_base_filenames");
    remove("/tmp/dfc_all_filenames");
    remove("/tmp/dfc_sorted_filenames");
    free(filenames);
    free(cmdBuffer);
    free(response);
}

void captureCmdOutput(char *cmd, char *buf, int bufsize)
{
    FILE *fp;
    fp = popen(cmd, "r");
    if (fp != NULL)
    {
        bzero(buf, bufsize);
        char line[256];
        int i = 0;
        // can't use putFileInBuffer because we can't
        // seek on a PIPE
        while (fgets(line, sizeof(line), fp))
        {

            strcpy(buf + i, line);
            i += strlen(line);
        }
        fclose(fp);
    }
}

void serverList(int connfd, char *dir)
{
    char *cmdOutput = (char *)malloc(BUFFER_SIZE);
    char cmd[256];
    mkdir(dir, 0775);
    sprintf(cmd, "ls %s", dir);
    captureCmdOutput(cmd, cmdOutput, BUFFER_SIZE);
    writeToSocket(connfd, cmdOutput, BUFFER_SIZE);
    free(cmdOutput);
}