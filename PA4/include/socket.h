// #include "../include/str_utils.h"
#include "../include/utils.h"

int get_socket(char *hostname, int port);

void writeToSocket(int sockfd, char *buffer, int size);
void readFromSocket(int sockfd, char *buffer, int size);

void sendChunk(int sockfd, char *filename, int id, char *chunk, int chunkSize);