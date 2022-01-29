/* 
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define BUFSIZE 1024

/* 
 * error - wrapper for perror
 */
void error(char *msg)
{
  perror(msg);
  exit(0);
}

void send_msg(int sockfd, char *buf, struct sockaddr_in addr)
{
  /* 
     * sendto: echo the input back to the client 
     */
  int len;
  int n;
  len = strlen(buf);

  n = sendto(sockfd, buf, strlen(buf), 0,
             (struct sockaddr *)&addr, sizeof(addr));
  if (n < 0)
    error("ERROR in sendto");
}

// https://idiotdeveloper.com/file-transfer-using-udp-socket-in-c/
void send_file(FILE *fp, char *buf, int sockfd, struct sockaddr_in addr)
{
  int n; // # of bytes sent at a time
  bzero(buf, BUFSIZE);
  strcpy(buf, "START");
  send_msg(sockfd, buf, addr);
  // clear any data currently in our buffer
  bzero(buf, BUFSIZE);
  while (fgets(buf, BUFSIZE, fp) != NULL)
  {
    printf("Sending data...");

    n = sendto(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&addr, sizeof(addr));
    if (n == -1)
    {
      error("Error in file transfer");
    }
    bzero(buf, BUFSIZE);
  }

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

int main(int argc, char **argv)
{
  int sockfd, portno, n;
  int serverlen;
  struct sockaddr_in serveraddr;
  struct hostent *server;
  char *hostname;
  char buf[BUFSIZE];

  /* check command line arguments */
  if (argc != 3)
  {
    fprintf(stderr, "usage: %s <hostname> <port>\n", argv[0]);
    exit(0);
  }
  hostname = argv[1];
  portno = atoi(argv[2]);

  /* socket: create the socket */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
    error("ERROR opening socket");

  /* gethostbyname: get the server's DNS entry */
  server = gethostbyname(hostname);
  if (server == NULL)
  {
    fprintf(stderr, "ERROR, no such host as %s\n", hostname);
    exit(0);
  }

  /* build the server's Internet address */
  bzero((char *)&serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  bcopy((char *)server->h_addr,
        (char *)&serveraddr.sin_addr.s_addr, server->h_length);
  serveraddr.sin_port = htons(portno);

  /* get a message from the user */
  while (1)
  {

    bzero(buf, BUFSIZE);
    printf("cmd: ");
    fgets(buf, BUFSIZE, stdin);

    /* send the message to the server */
    serverlen = sizeof(serveraddr);
    n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
    if (n < 0)
      error("ERROR in sendto");

    /* print the server's reply */
    bzero(buf, BUFSIZE);
    n = recvfrom(sockfd, buf, BUFSIZE, 0, &serveraddr, &serverlen);
    if (n < 0)
      error("ERROR in recvfrom");

    /* Check for goodbye msg */
    if (strcmp(buf, "Goodbye") == 0)
    {
      printf("\n%s\n", buf);
      exit(0);
    }
    else if (strcmp(buf, "START") == 0)
    {
      // the server is trying to send us a file!
      write_file(sockfd, buf, serveraddr);
      printf("Wrote file to cwd\n");
    }
    else
    {
      printf("\n%s\n", buf);
    }
  }
  return 0;
}
