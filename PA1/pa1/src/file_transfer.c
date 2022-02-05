#include "../include/file_transfer.h"

// https://idiotdeveloper.com/file-transfer-using-udp-socket-in-c/
void send_file(FILE *fp, char *filename, char *buf, int sockfd, struct sockaddr_in addr)
{
    int n; // # of bytes sent at a time
    // what length is our file?
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    // start transfer
    char *startstr = (char *)malloc(BUFSIZE);
    bzero(startstr, BUFSIZE);
    snprintf(startstr, BUFSIZE, "START %ld %s", fsize, filename);
    // clear any data currently in our buffer
    bzero(buf, BUFSIZE);
    strcpy(buf, startstr);
    send_msg(sockfd, buf, addr);
    free(startstr);

    printf("Sending data...\n");

    n = fread(buf, fsize, 1, fp);
    if (n != -1)
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
    int n;
    int max_filename_char = 100;
    char *filename;
    int fsize; // this will be included after keyword START
    chopN(buf, strlen("START") + 1);
    fsize = strtol(buf, &filename, 10); // convert to int, base ten
    filename = strstrip(filename);
    // Creating a file.
    fp = fopen(filename, "wb");

    // Receiving the data and writing it into the file.
    bzero(buf, BUFSIZE);

    while (1)
    {
        socklen_t clilen = sizeof(addr);
        n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&addr, &clilen);

        if (strcmp(buf, "END") == 0)
        {
            bzero(buf, BUFSIZE);
            break;
            return;
        }
        // write recieved data to buffer
        fwrite(buf, n, 1, fp);
        if (n > 0)
        {
            bzero(buf, BUFSIZE);
        }
    }

    fclose(fp);
    return;
}