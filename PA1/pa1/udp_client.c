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
#include <stdbool.h>

#define BUFSIZE 1024

/* 
 * error - wrapper for perror
 */
void error(char *msg)
{
  perror(msg);
  exit(0);
}

// https://stackoverflow.com/a/15515276
bool startsWith(const char *a, const char *b)
{
  if (strncmp(a, b, strlen(b)) == 0)
    return 1;
  return 0;
}

// https://stackoverflow.com/a/1488419
char *strstrip(char *s)
{
  size_t size;
  char *end;

  size = strlen(s);

  if (!size)
    return s;

  end = s + size - 1;
  while (end >= s && isspace(*end))
    end--;
  *(end + 1) = '\0';

  while (*s && isspace(*s))
    s++;

  return s;
}

// https://stackoverflow.com/a/4761840
size_t chopN(char *str, size_t n)
{
  // assert(n != 0 && str != 0);
  // char temp[n+1];
  // memccpy(temp, str, n);
  size_t len = strlen(str);
  if (n > len)
    n = len;
  memmove(str, str + n, len - n + 1);
  // bzero(str + n - 1, BUFSIZE - n);
  return (len - n);
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
    // Check command that was sent. Respond accordingly
    char *stripped;
    // strip string of leading/trailing spaces
    stripped = strstrip(buf);
    if (startsWith(stripped, "put"))
    {
      // remove delete keyword to obtain filename
      size_t len;
      len = chopN(stripped, strlen("put") + 1);
      // strip string of leading/trailing spaces
      stripped = strstrip(stripped);
      if (strlen(stripped) == 0)
      {
        // case where user sends "delete" but no filenames
        printf("Usage: put must be followed by a filename\n");
        continue;
      }
      else
      {
        // printf("Filename to put %s\n", stripped);
        FILE *fp;
        fp = fopen(stripped, "r");
        if (fp == NULL)
        {
          printf("Error opening specified file. It may not exist on this host\n");
          continue;
        }
        else
        {
          // file open successful. Send file
          send_file(fp, buf, sockfd, serveraddr);
          continue;
        }
      }
    }
    else
    {
      /* send the message to the server */
      serverlen = sizeof(serveraddr);
      n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
      if (n < 0)
        error("ERROR in sendto");
    }

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
