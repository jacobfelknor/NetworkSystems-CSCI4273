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

// https://idiotdeveloper.com/file-transfer-using-udp-socket-in-c/
void send_file(FILE *fp, char *buf, int sockfd, struct sockaddr_in addr)
{
  int n; // # of bytes sent at a time
  bzero(buf, BUFSIZE);
  strcpy(buf, "START");
  send_msg(sockfd, buf, addr);
  // clear any data currently in our buffer
  bzero(buf, BUFSIZE);
  // while (fgets(buf, BUFSIZE, fp) != NULL)
  // {
  //   printf("Sending data...");

  //   n = sendto(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&addr, sizeof(addr));
  //   if (n == -1)
  //   {
  //     error("Error in file transfer");
  //   }
  //   bzero(buf, BUFSIZE);
  // }

  // let other side know that we've finished sending data
  // strcpy(buf, "END");
  // n = sendto(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&addr, sizeof(addr));
  // if (n == -1)
  // {
  //   error("Error sending 'END' msg");
  // }
  // send_msg(sockfd, buf, addr);
  // fclose(fp);
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
  bzero(buf, BUFSIZ);
  while (1)
  {

    n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&addr, sizeof(addr));

    if (strcmp(buf, "END") == 0)
    {
      break;
      return;
    }
    // write recieved data to buffer
    fprintf(fp, "%s", buf);
    bzero(buf, BUFSIZ);
  }

  fclose(fp);
  return;
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
    char *stripped;
    // strip string of leading/trailing spaces
    stripped = strstrip(buf);
    if (strcmp(stripped, "exit") == 0)
    {
      bzero(buf, BUFSIZE);
      strncpy(buf, "Goodbye", sizeof buf - 1);
      send_msg(sockfd, buf, clientaddr);
    }
    else if (strcmp(stripped, "ls") == 0)
    {
      captureCmdOutput("ls", buf);
      send_msg(sockfd, buf, clientaddr);
    }
    else if (startsWith(stripped, "delete"))
    {
      // remove delete keyword to obtain filename
      size_t len;
      len = chopN(stripped, strlen("delete") + 1);
      // strip string of leading/trailing spaces
      stripped = strstrip(stripped);
      if (strlen(stripped) == 0)
      {
        // case where user sends "delete" but no filenames
        strncpy(buf, "Usage: delete must be followed by a filename", sizeof buf - 1);
      }
      else
      {
        // delete followed by a string, attempt to delete that file.
        // file exists, delete it
        char *cmd = (char *)malloc(BUFSIZE);
        // redirect stderr to stdout so that popen will capture it
        snprintf(cmd, BUFSIZE, "rm -v 2>&1 %s", stripped);
        captureCmdOutput(cmd, buf);
        free(cmd);
        send_msg(sockfd, buf, clientaddr);
      }
      // bzero(buf, BUFSIZE);
    }
    else if (startsWith(stripped, "get"))
    {
      // remove get keyword to obtain filename
      size_t len;
      len = chopN(stripped, strlen("get") + 1);
      // strip string of leading/trailing spaces
      stripped = strstrip(stripped);
      if (strlen(stripped) == 0)
      {
        // case where user sends "delete" but no filenames
        strncpy(buf, "Usage: get must be followed by a filename", sizeof buf - 1);
      }
      else
      {
        FILE *fp;
        fp = fopen(stripped, "r");
        send_file(fp, buf, sockfd, clientaddr);
      }
    }
    else
    {
      // bzero(buf, BUFSIZ);
      strcpy(buf, "Unrecognized cmd. Options are\n\nget, put, delete, ls, exit\n\nTry again\n");
      send_msg(sockfd, buf, clientaddr);
    }

    //   /*
    //    * sendto: echo the input back to the client
    //    */
    //   int len;
    //   len = strlen(buf);

    //   n = sendto(sockfd, buf, strlen(buf), 0,
    //              (struct sockaddr *)&clientaddr, clientlen);
    //   if (n < 0)
    //     error("ERROR in sendto");
  }
}
