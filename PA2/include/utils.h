#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdbool.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <errno.h>

#define RESPONSE_FILE_BUFFER 1048576
#define RESPONSE_BUFFER_SIZE RESPONSE_FILE_BUFFER + 100

void error(char *msg);

void reply(int connfd, char *path, char *requestMethod, char *httpVersion);

void request2buffer(int connfd, char *buf, int bufsize);

void putFileInBuffer(char *buf, int bufsize, FILE *f);

long getFileSize(FILE *f);

int isDirectory(const char *path);

char *getFileExtension(char *path);

char *getContentType(char *path);

void sendResponse(int connfd, char *responseBuffer, long responseSize);

long buildResponse(char *responseBuffer, char *httpVersion, char *statusCode, char *contentType, long contentLength);

long appendContent(char *responseBuffer, char *fileBuffer, long fileSize);
