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
#include <sys/time.h>
#include <errno.h>
#include <openssl/md5.h>

#include "../include/socket.h"

#define BUFFER_SIZE 1048576

long putFileInBuffer(char *buf, int bufsize, FILE *f);
void putBufferInFile(char *buf, int bufsize, FILE *f);
void request2buffer(int connfd, char *buf, int bufsize);
long getFileSize(FILE *f);
void parseRequest(char **request, char *cmd, char *filename, int *chunkSize);
char *pathConcat(char *s1, char *s2);
void serverPutFile(int sockfd, char *request, char *cmd, char *dir, char *filename, int chunkSize);
void clientPutFile(char *path, char *buffer, int *socks, char *filename);
void clientGetFile(char **servers, int *ports, int *socks, char *filename, char *cmd);
void serverGetFile(int connfd, char *dir, char *filename, char *cmd);
void splitHost(char *line, char *host, int *port);
void clientList(char **servers, int *ports, int *socks);
void serverList(int connfd, char *dir);
void captureCmdOutput(char *cmd, char *buf, int bufsize);