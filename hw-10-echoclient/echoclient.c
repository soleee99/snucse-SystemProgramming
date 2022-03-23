
// standard headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// for networking
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>

#define DEFAULT_PORT 1522
#define DEFAULT_HOST "localhost"
#define NET_RECV 0
#define NET_SEND 1

static char stx_str[] = "client <host> [<port>]";

static int sock_fd = -1;              ///< listening socket, closed in atexit()

void error(const char *msg){
  if (msg) fprintf(stderr, "Error: %s\n", msg);
  else fprintf(stderr, "Error.\n");
  exit(EXIT_FAILURE);
}

void syntax(const char *msg, const char *syntax){
  if(msg) fprintf(stderr, "%s\n\n", msg);
  if(syntax) fprintf(stderr, "Syntax: %s\n", syntax);
  else fprintf(stderr, "Syntax error.\n");
  exit(EXIT_FAILURE);
}

struct addrinfo *getsocklist(const char *host, unsigned short port, int family, int type, int listening, int *res){
  char portstr[6];
  struct addrinfo hints, *ai;
  int r;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = family;
  hints.ai_socktype = type;
  hints.ai_flags = AI_ADDRCONFIG;
  
  if(listening) hints.ai_flags |= AI_PASSIVE;
  hints.ai_protocol = 0;  //TODO: why 0

  snprintf(portstr, sizeof(portstr), "%d", port);
  r = getaddrinfo(listening ? NULL : host, portstr, &hints, &ai); // this function returns potental usabel sockets

  if(res) *res = r;
  if(r !=0) return NULL;
  else return ai;
}

void dump_sockaddr(struct sockaddr *sa){
  char adrstr[40];

  if(sa->sa_family == AF_INET){
    //IPv4
    struct sockaddr_in *sa4 = (struct sockaddr_in*) sa;
    if(inet_ntop(sa4->sin_family, &sa4->sin_addr, adrstr, sizeof(adrstr)) == NULL){
      perror("inet_pton");
    } else {
      printf("%s:%d (IPv4)", adrstr, ntohs(sa4->sin_port));
    }
  } else if (sa->sa_family == AF_INET6){
    //IPv6
    struct sockaddr_in6 *sa6 = (struct sockaddr_in6*) sa;
    if(inet_ntop(sa6->sin6_family, &sa6->sin6_addr, adrstr, sizeof(adrstr)) == NULL){
      perror("inet_pton");
    } else {
      printf("%s:%d (IPv6)", adrstr, ntohs(sa6->sin6_port));
    }
  } else {
    // unsupported
    printf("unknown protcol family (neither IPv4 nor IPv6)\n");
  }
  fflush(stdout);
}

static int transfer_data(int mode, int sock, char *buf, size_t len){
  if (!((mode == NET_RECV) || (mode == NET_SEND)) || (buf == NULL)) return -2;

  int res = 0;

  while(len > 0){
    int r;
    if (mode == NET_RECV) r = recv(sock, buf, len, 0);
    else r = send(sock ,buf, len, 0);

    if(r > 0){
      // success: read r bytes
      buf += r;
      len -= r;
      res += r;
    } else if(r == 0){
      break;
    } else{
      if (errno == EINTR) continue;
      res = -1;
      break;
    }
  }
  return res;
}

int get_data(int sock, char *buf, size_t len){
  return transfer_data(NET_RECV, sock, buf, len);
}

int put_data(int sock, char *buf, size_t len){
  return transfer_data(NET_SEND, sock, buf, len);
}

int get_line(int sock, char **buf, size_t *cur_len){
  if (*cur_len == 0) return -2;

  char c;
  int res = 0;
  size_t pos = 0;

  //read to first newline or transmission error
  do{
    res = get_data(sock, &c, 1);
    if(res == 1){
      (*buf) [pos++] = c;

      if(pos == *cur_len){
        *cur_len <<= 1;
        *buf = (char*)realloc(*buf, *cur_len);
      }
    }
  } while((res == 1) && (c != '\n'));

  (*buf)[pos] = '\0';

  if (c == '\n') return (int) pos;
  else return res;
}

int put_line(int sock, char *buf, size_t len){
  if(len == 0) return -2;

  char c;
  int res = 1, res2;
  size_t pos = 0;

  while((pos < len) && (buf[pos] != '\0')) pos++;

  if(pos > 0){
    res = put_data(sock, buf, pos);
  }
  if((res > 0) && (buf[pos-1] != '\n')){
    c = '\n';
    res2 = put_data(sock, &c, 1);
    if(res2 < 0) res = res2;
    else res += res2;
  }
  return res;
}

/// @brief connect to @a host at port @a port
///
/// Returns the file descriptor of the connected socket
/// or aborts (does not return) on failure.
///
/// @param host hostname of server
/// @param port port of server
/// @retval int file descriptor of connected socket
int connect_to(char *host, uint16_t port)
{
  struct addrinfo *ai, *ai_it;  // structure used by getaddrinfo
  int fd = -1;

  printf("Connecting to %s:%d...\n", host, port);

  //
  // get list of potential sockets
  //
  ai = getsocklist(host, port, AF_UNSPEC, SOCK_STREAM, 0, NULL);  
  // AD_UNSPEC option returns all possible addresses.

  // iterate through potential addressinfo structs and try to connect to server
  ai_it = ai;
  while (ai_it != NULL) {
    printf("  trying "); dump_sockaddr(ai_it->ai_addr); printf("..."); fflush(stdout);

    fd = socket(ai_it->ai_family, ai_it->ai_socktype, ai_it->ai_protocol);
    if (fd != -1) {
      if (connect(fd, ai_it->ai_addr, ai_it->ai_addrlen) != -1) break; // success, leave
      close(fd);
    }
    printf("failed.\n");
    ai_it = ai_it->ai_next;
  }

  // ai_it == NULL -> connection failed, abort
  if (ai_it == NULL) error("Cannot connect.");

  freeaddrinfo(ai);

  printf("success.\n");
  return fd;
}

/// @brief main client routine: read input line from terminal, send it to
///        server and print reply
/// @param connfd connected socket
void run_client(int connfd)
{
  char *msg = NULL;
  size_t msg_len = 0;

  if (isatty(STDIN_FILENO)) printf("Press Ctrl-C to exit.\n");

  while (1) {
    ssize_t res;
    // read a line of input
    printf("Enter string of hit Enter to quit: "); fflush(stdout);
    if ((res = getline(&msg, &msg_len, stdin)) > 0) {
      int res;

      // send line to server
      res = put_line(connfd, msg, msg_len);
      if (res < 0) printf("Error: cannot send data to server (%d).\n", res);
      else {
        // read server reply
        printf("Reply from server: "); fflush(stdout);
        res = get_line(connfd, &msg, &msg_len);
        if (res <= 0) printf("Error: cannot read reply from server (%d).\n", res);
        else printf("%s", msg);
      }
    } else break;
  }

  printf("End of input reached.\n");
  free(msg);
}


/// @brief close main connection (atexit() handler)
void close_connection(void)
{
  if (sock_fd != -1) {
    close(sock_fd);
    sock_fd = -1;
  }
}

/// @brief program entry point
int main(int argc, char *argv[])
{
  char *host = argc > 1 ? argv[1] : DEFAULT_HOST;   // this comes in as csap.snu.ac.kr
  uint16_t port = argc > 2 ? atoi(argv[2]) : DEFAULT_PORT;  // this comes in as 64321, where the echo server is running

  if (port > 0xffff) syntax("Port must be in range 0-65535.", stx_str);

  // connect to host:port
  sock_fd = connect_to(host, port); // returns the file descriptor of listening socket
  //atexit(close_connection);

  // run the telnet client
  run_client(sock_fd);
  

  // that's all, folks
  return EXIT_SUCCESS;
}
