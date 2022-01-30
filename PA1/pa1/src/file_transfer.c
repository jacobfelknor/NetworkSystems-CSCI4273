#include "../include/file_transfer.h"

// https://idiotdeveloper.com/file-transfer-using-udp-socket-in-c/
void send_file(FILE *fp, char *buf, int sockfd, struct sockaddr_in addr)
{
    int n; // # of bytes sent at a time
    bzero(buf, BUFSIZE);
    // clear any data currently in our buffer
    bzero(buf, BUFSIZE);
    // what length is our file?
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    // start transfer
    char *startstr = (char *)malloc(BUFSIZE);
    snprintf(startstr, BUFSIZE, "START %ld", fsize);
    strcpy(buf, startstr);
    send_msg(sockfd, buf, addr);
    free(startstr);

    printf("Sending data...\n");

    n = fread(buf, fsize, 1, fp);
    if(n != -1)
    {

        n = sendto(sockfd, buf, fsize, 0, (struct sockaddr *)&addr, sizeof(addr));
        if (n == -1)
        {
            error("Error in file transfer");
        }
        bzero(buf, BUFSIZE);
    }
    printf("Sent.\n");
    // let other side know that we've finished sending data
    strcpy(buf, "END");
    // n = sendto(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&addr, sizeof(addr));
    send_msg(sockfd, buf, addr);
    bzero(buf, BUFSIZE);
    fclose(fp);
}
// https://idiotdeveloper.com/file-transfer-using-udp-socket-in-c/
void write_file(int sockfd, char *buf, struct sockaddr_in addr)
{
    FILE *fp;
    char *filename = "test.txt";
    int n;
    int fsize; // this will be included after keyword START
    chopN(buf, strlen("START") + 1);
    fsize = strtol(buf, buf, 10);

    // Creating a file.
    fp = fopen(filename, "wb");

    // Receiving the data and writing it into the file.
    bzero(buf, BUFSIZE);
    
    while (1)
    {

        n = recvfrom(sockfd, buf, fsize, 0, (struct sockaddr *)&addr, sizeof(addr));

        if (strcmp(buf, "END") == 0)
        {
            bzero(buf, BUFSIZE);
            break;
            return;
        }
        // write recieved data to buffer
        fwrite(buf, fsize, 1, fp);
        bzero(buf, BUFSIZE);
    }

    fclose(fp);
    return;
}