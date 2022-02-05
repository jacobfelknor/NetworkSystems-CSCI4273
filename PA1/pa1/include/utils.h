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

void error(char *msg);

void send_msg(int sockfd, char *buf, struct sockaddr_in addr);

void get_msg_timeout(int sockfd, char *buf, struct sockaddr_in addr);

void putFileInBuffer(char *buf, FILE *f);

void captureCmdOutput(char *cmd, char *buf);

char *sgets(char *s, int n, const char **strp);