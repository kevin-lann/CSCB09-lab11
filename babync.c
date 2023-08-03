#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

void parse_inaddr(struct addrinfo *ai, const char *hostname, const char *port) {
  struct addrinfo hint;
  struct addrinfo *head;

  memset(&hint, 0, sizeof(struct addrinfo));
  hint.ai_family = AF_INET;
  hint.ai_socktype = SOCK_STREAM;
  hint.ai_protocol = 0;
  hint.ai_flags = AI_NUMERICSERV;

  int r = getaddrinfo(hostname, port, &hint, &head);
  if (r != 0) {
    if (r == EAI_SYSTEM) {
      perror("getaddrinfo");
    } else {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(r));
    }
    exit(1);
  } else {
    *ai = *head;
    ai->ai_next = NULL;
    freeaddrinfo(head);
  }
}


// dot notation IP address, port
int main(int argc, char **argv)
{
  if (argc < 3) {
    fprintf(stderr, "Need IPv4 address and port.\n");
    return 1;
  }

  struct addrinfo ai;
  parse_inaddr(&ai, argv[1], argv[2]);

  int s = socket(ai.ai_family, ai.ai_socktype, ai.ai_protocol);
  if (-1 == connect(s, ai.ai_addr, ai.ai_addrlen)) {
    perror("connect");
    return 1;
  }

  // TODO: A select() loop to copy stdin to socket (s), copy socket to stdout.
  // Do not exit until both stdin and socket are EOF.
  // Do not monitor an FD for reading if it reached EOF in the past, lest you
  // would cause busy-polling. (Why?)

  fd_set writefds;

  while(1) {
    FD_ZERO(&writefds);
    FD_SET(0, &writefds);
    FD_SET(s, &writefds);
    while(select(s+1, &writefds, NULL, NULL, NULL) == -1);
    if(FD_ISSET(0, &writefds)) {
        char input[1024];
        if(read(0, input, 1023) != 0) write(s, input, sizeof(input));
        else break; // stdin has reached EOF 
    }
    if(FD_ISSET(s, &writefds)) {
        char input[1024];
        if(read(s, input, 1023) != 0) write(1, input, sizeof(input));
        else break; // socket has reached EOF 
    }

  }

  close(s);
  return 0;
}