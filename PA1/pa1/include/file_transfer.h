#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/limits.h>
#include <stdbool.h>
#include "./constants.h"
#include "./utils.h"
#include "./str_utils.h"

// https://idiotdeveloper.com/file-transfer-using-udp-socket-in-c/
void send_file(FILE *fp, char *buf, int sockfd, struct sockaddr_in addr);
// https://idiotdeveloper.com/file-transfer-using-udp-socket-in-c/
void write_file(int sockfd, char *buf, struct sockaddr_in addr);