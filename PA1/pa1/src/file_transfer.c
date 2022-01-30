#include "../include/file_transfer.h"

// https://idiotdeveloper.com/file-transfer-using-udp-socket-in-c/
void send_file(FILE *fp, char *buf, int sockfd, struct sockaddr_in addr)
{
    int n; // # of bytes sent at a time
    bzero(buf, BUFSIZE);
    strcpy(buf, "START");
    send_msg(sockfd, buf, addr);
    // clear any data currently in our buffer
    bzero(buf, BUFSIZE);
    printf("Sending data...\n");
    while (fgets(buf, BUFSIZE, fp) != NULL)
    {

        n = sendto(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&addr, sizeof(addr));
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

    // Creating a file.
    fp = fopen(filename, "w");

    // Receiving the data and writing it into the file.
    bzero(buf, BUFSIZE);
    while (1)
    {

        n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&addr, sizeof(addr));

        if (strcmp(buf, "END") == 0)
        {
            bzero(buf, BUFSIZE);
            break;
            return;
        }
        // write recieved data to buffer
        fprintf(fp, "%s", buf);
        bzero(buf, BUFSIZE);
    }

    fclose(fp);
    return;
}