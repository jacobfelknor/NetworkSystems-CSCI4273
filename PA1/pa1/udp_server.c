/* 
 * udpserver.c - A simple UDP echo server 
 * usage: udpserver <port>
 */

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

#define BUFSIZE 1024

/*
 * error - wrapper for perror
 */
void error(char *msg)
{
  perror(msg);
  exit(1);
}

void putFileInBuffer(char *buf, FILE *f)
{
  // clear buffer first
  bzero(buf, BUFSIZE);
  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET); /* same as rewind(f); */

  // char *string = malloc(fsize + 1);
  // if (fsize > sizeof buf - 1)
  // {
  //   // only fill buffer
  //   fsize = sizeof buf - 1;
  // }
  fread(buf, fsize, 1, f);
  fclose(f);

  // string[fsize] = 0;
}

// https://stackoverflow.com/a/15515276
bool startsWith(const char *a, const char *b)
{
  if (strncmp(a, b, strlen(b)) == 0)
    return 1;
  return 0;
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

void captureCmdOutput(char *cmd, char *buf)
{
  FILE *fp;
  int status;
  char path[PATH_MAX];

  fp = popen(cmd, "r");
  if (fp == NULL)
    /* Handle error */;
  // clear buffer first, then store contents of ls
  putFileInBuffer(buf, fp);
}

int main(int argc, char **argv)
{
  int sockfd;                    /* socket */
  int portno;                    /* port to listen on */
  int clientlen;                 /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp;         /* client host info */
  char buf[BUFSIZE];             /* message buf */
  char *hostaddrp;               /* dotted decimal host addr string */
  int optval;                    /* flag value for setsockopt */
  int n;                         /* message byte size */

  /* 
   * check command line arguments 
   */
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);

  /* 
   * socket: create the parent socket 
   */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
    error("ERROR opening socket");

  /* setsockopt: Handy debugging trick that lets 
   * us rerun the server immediately after we kill it; 
   * otherwise we have to wait about 20 secs. 
   * Eliminates "ERROR on binding: Address already in use" error. 
   */
  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
             (const void *)&optval, sizeof(int));

  /*
   * build the server's Internet address
   */
  bzero((char *)&serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);

  /* 
   * bind: associate the parent socket with a port 
   */
  if (bind(sockfd, (struct sockaddr *)&serveraddr,
           sizeof(serveraddr)) < 0)
    error("ERROR on binding");

  /* 
   * main loop: wait for a datagram, then echo it
   */
  clientlen = sizeof(clientaddr);
  while (1)
  {

    /*
     * recvfrom: receive a UDP datagram from a client
     */
    bzero(buf, BUFSIZE);
    n = recvfrom(sockfd, buf, BUFSIZE, 0,
                 (struct sockaddr *)&clientaddr, &clientlen);
    if (n < 0)
      error("ERROR in recvfrom");

    /* 
     * gethostbyaddr: determine who sent the datagram
     */
    hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
                          sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    if (hostp == NULL)
      error("ERROR on gethostbyaddr");
    hostaddrp = inet_ntoa(clientaddr.sin_addr);
    if (hostaddrp == NULL)
      error("ERROR on inet_ntoa\n");
    printf("server received datagram from %s (%s)\n",
           hostp->h_name, hostaddrp);
    printf("server received %d/%d bytes: %s\n", strlen(buf), n, buf);
    // Check command that was sent. Respond accordingly
    if (strcmp(buf, "exit\n") == 0)
    {
      bzero(buf, BUFSIZE);
      strncpy(buf, "Goodbye", sizeof buf - 1);
    }
    else if (strcmp(buf, "ls\n") == 0)
    {
      captureCmdOutput("ls", buf);
      // while (fgets(path, PATH_MAX, fp) != NULL)
      // {
      //   printf("%s", path);
      // }
    }
    else if (startsWith(buf, "delete"))
    {
      // remove delete keyword to obtain filename
      size_t len;
      len = chopN(buf, strlen("delete") + 1);
      if (len == 0)
      {
        // case where user sends "delete" but no filenames
        strncpy(buf, "Usage: delete must be followed by a filename", sizeof buf - 1);
      }
      else
      {
        // delete followed by a string, attempt to delete that file.
        // check if file exists, return msg if it doesn't
        printf("%s\n", buf);
        // file exists, delete it
      }
      // bzero(buf, BUFSIZE);
    }

    /* 
     * sendto: echo the input back to the client 
     */
    int len;
    len = strlen(buf);

    n = sendto(sockfd, buf, strlen(buf), 0,
               (struct sockaddr *)&clientaddr, clientlen);
    if (n < 0)
      error("ERROR in sendto");
  }
}
