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

#define BUFFER_SIZE 1048576

void error(char *msg);

long putFileInBuffer(char *buf, int bufsize, FILE *f);
void putBufferInFile(char *buf, int bufsize, FILE *f);

long getFileSize(FILE *f);
