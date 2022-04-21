// #include "../include/str_utils.h"
// #include "../include/utils.h"
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

void error(char *msg);

int get_socket(char *hostname, int port);

void writeToSocket(int sockfd, char *buffer, int size);
void readFromSocket(int sockfd, char *buffer, int size);
void readLineFromSocket(int sockfd, char *buffer, int size);

void sendChunk(int sockfd, char *filename, int id, char *chunk, int chunkSize);