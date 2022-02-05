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

#include "../include/str_utils.h"
#include "../include/constants.h"
#include "../include/file_transfer.h"
#include "../include/utils.h"

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
        send_msg(sockfd, buf, clientaddr);
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
        // case where user sends "get" but no filenames
        strncpy(buf, "Usage: get must be followed by a filename", sizeof buf - 1);
        send_msg(sockfd, buf, clientaddr);
      }
      else
      {
        FILE *fp;
        fp = fopen(stripped, "rb");
        if (fp == NULL)
        {
          strcpy(buf, "Error opening specified file. It may not exist on this server");
          send_msg(sockfd, buf, clientaddr);
        }
        else
        {
          // file open successful. Send file
          send_file(fp, stripped, buf, sockfd, clientaddr);
        }
      }
    }
    else if (startsWith(stripped, "START"))
    {
      // the client is trying to send us a file!
      write_file(sockfd, buf, clientaddr);
      printf("Wrote file to cwd\n");
    }
    else
    {
      // bzero(buf, BUFSIZ);
      strcpy(buf, "Unrecognized cmd. Options are\n\nget, put, delete, ls, exit\n\nTry again");
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
