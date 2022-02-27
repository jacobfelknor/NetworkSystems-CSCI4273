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

#define RESPONSE_FILE_BUFFER 1049000
#define RESPONSE_BUFFER_SIZE RESPONSE_FILE_BUFFER + 100

void error(char *msg);

void reply(int connfd, char *path, char *httpVersion);

void request2buffer(int connfd, char *buf, int bufsize);

void putFileInBuffer(char *buf, int bufsize, FILE *f);

long getFileSize(FILE *f);

char *getFileExtension(char *path);

char *getContentType(char *path);

// void send_msg(int sockfd, char *buf, struct sockaddr_in addr);

// void get_msg_timeout(int sockfd, char *buf, struct sockaddr_in addr);

// void captureCmdOutput(char *cmd, char *buf);

// char *sgets(char *s, int n, const char **strp);