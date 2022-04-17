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

#define RESPONSE_FILE_BUFFER 1048576
#define RESPONSE_BUFFER_SIZE RESPONSE_FILE_BUFFER + 100

void error(char *msg);

long putFileInBuffer(char *buf, int bufsize, FILE *f);
void putBufferInFile(char *buf, int bufsize, FILE *f);

long getFileSize(FILE *f);