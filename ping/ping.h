#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <limits.h>
#include <math.h>
#include <ctype.h>

#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/cdefs.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX(a,b) ((a>b)?a:b)
#define MIN(a,b) ((a<b)?a:b)

#define SOL_IP IPPROTO_IP
#define PING_BYTES 64
#define PORT_NO 43543
#define PING_DELAY 1
#define TIMEOUT 2
#define MAX_HOST 1024
#define TTL 32
#define ID 5446

struct icmphdr{
  u_int8_t type;
  u_int8_t code;
  u_int16_t checkSum;
  union{
    struct{
      u_int16_t        id;
      u_int16_t        sequence;
    } echo;
    u_int32_t        gateway;
    struct{
      u_int16_t        mtu;
    } frag;
  } un;
};

int interrupt = 0;
int ping_count = 0;

void handle_interrupt(int sig);
void argParser(int argc, char *argv[], int *count, char **hostname);
unsigned short checkSum(short *data, size_t bytes);
void ping(int sockfd, struct sockaddr_in *dest, char *ip_addr, char*hostname);
