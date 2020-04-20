#include "ping.h"

void handle_interrupt(int sig) {
  if (sig == SIGINT) printf("\n-- Program End --\n");
  printf("received SIGINT\n");
  interrupt=1;
}

void argParser(int argc, char *argv[], int *count, char **hostname) {
  int arg_c = 1;
  int ch, ind_len, len;
	for(; arg_c<argc; arg_c++) {
		switch(argv[arg_c][0]) {
			case '-':
				len = strlen(argv[arg_c]);
        ind_len=1;
				for(; ind_len<len; ind_len++) {
					ch = (int)argv[arg_c][ind_len];
					switch(ch) {
						case 'c':
							arg_c++;
							*count = atoi(argv[arg_c]);
							break;
						default:
							fprintf(stderr, "USAGE: sudo %s [-t count] <hostname>\n", argv[0]);
					}
				}
				break;
			default:
				*hostname = argv[arg_c];
		}
	}
}

unsigned short checkSum(short *data, size_t bytes){
	if (bytes%2==1) {printf(stderr, "ICMP_checksum: must be even number of bytes %zu\n", bytes); return -1;}

	unsigned short out;
	unsigned int sum = 0;

	for(sum=0; bytes>1; bytes-=2) sum += *data++;

  unsigned int right_shift = (sum >> 16);

	sum = right_shift + (sum & 0xFFFF);
	sum += right_shift;

	out = ~sum;
	return out;
}

char* lookupDNS(char *hostname, struct sockaddr_in *server) {
	struct hostent *host;
  host = gethostbyname(hostname);
  size_t size_char = sizeof(char);
	char *ip_addr = (char *)malloc(size_char*MAX_HOST);

	if(host == NULL) {fprintf(stderr, "Given host not found\n"); return NULL;}

	strcpy(ip_addr, inet_ntoa(*(struct in_addr *)host->h_addr));
	server->sin_family = host->h_addrtype;
	server->sin_addr.s_addr = *(uint32_t*) host->h_addr;
	server->sin_port = htons(PORT_NO);
	return ip_addr;
}


void ping(int sockfd, struct sockaddr_in *dest, char *ip_addr, char*hostname) {
	struct sockaddr_in receiver;
	struct timeval start, end;
  int ttl = TTL;
  int transmitted = 0;
  int received = 0;
  int status;
  int num_packets = 0;
  socklen_t receiver_addr_size;
	double min_time = INT_MAX;
  double max_time = 0;
  double mean = 0.0;
  double std_dev;
	float rtt_sum = 0.0;
  float rtt_sum_change = 0.0;

	status = setsockopt(sockfd, SOL_IP, IP_TTL, &ttl, sizeof(ttl));
	if(status != 0) {perror("Error\n"); fprintf(stderr, "Failed to modify TTL in socket options\n");return;}

	struct timeval timeout;
	bzero(&timeout, sizeof(timeout));
	timeout.tv_sec = TIMEOUT;

	status = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
	if(status != 0) {perror("Error\n"); fprintf(stderr, "Failed to modify timeout in socket options\n");return;}

	gettimeofday(&start, NULL);
	while(ping_count==0 || num_packets<ping_count) {
		num_packets++;
		struct timeval start_pkt, end_pkt;

		struct icmphdr hdr;
		bzero(&hdr, sizeof(hdr));

		hdr.type = ICMP_ECHO;
		hdr.code = 0;
		hdr.un.echo.id = htons(ID);
		hdr.un.echo.sequence = htons(num_packets);
		hdr.checkSum = 0;
		hdr.checkSum = checkSum((short *)&hdr, sizeof(hdr));

		sleep(PING_DELAY);

		gettimeofday(&start_pkt, NULL);

		status = sendto(sockfd, &hdr, sizeof(hdr), 0,(struct sockaddr *) dest, sizeof(*dest));

		int packet_sent = 1;
		if(status>0) transmitted++;
		else{
      fprintf(stdout, "status: %d\n", status);
			fprintf(stderr, "Cannot send packet.\n");
			packet_sent = 0;
    }

		receiver_addr_size = sizeof(receiver);
		status = recvfrom(sockfd, &hdr, sizeof(hdr), 0,(struct sockaddr *)&receiver, &receiver_addr_size);

		gettimeofday(&end_pkt, NULL);
		if(interrupt) break;

		if(!(status>0 || num_packets<=1)) {
			if(packet_sent) {
				received++;
				double time = (((((end_pkt.tv_sec - start_pkt.tv_sec) * 1000000) + end_pkt.tv_usec) - (start_pkt.tv_usec))/1000.0);
				min_time = MIN(min_time, time);
				max_time = MAX(max_time, time);
				rtt_sum += time;
				rtt_sum_change += time*time;
				fprintf(stdout, "%d bytes from %s: icmp_seq=%d ttl=%d time=%0.1f ms\n", PING_BYTES, ip_addr, num_packets, TTL, time);
			}
		}
    else{perror("Error"); fprintf(stderr, "Didn't receive packet.\n");}

	}
	gettimeofday(&end, NULL);

	double time_milli = ((((end.tv_sec - start.tv_sec) * 1000000) + end.tv_usec) - (start.tv_usec))/1000.0;
	mean = rtt_sum/received;
	rtt_sum /= received;
	rtt_sum_change /= (received);
	std_dev = sqrt(rtt_sum_change - rtt_sum*rtt_sum);

	if(interrupt)	transmitted--;

	fprintf(stdout, "\n-- ping %s statistics --\n", hostname);
	fprintf(stdout, "%d packets transmitted, %d received, %0.0f%% packet loss, time %0.0fms\n", transmitted, received, (1.0*(transmitted-received)/transmitted*100), time_milli);
	fprintf(stdout, "rtt min/avg/max/mdev = %0.3f/%0.3f/%0.3f/%0.3f ms\n", min_time, max_time, mean, std_dev);

	status = close(sockfd);
	if(status!=0) {perror("Error\n"); fprintf(stderr, "Failed to close socket\n");	return;}
}

int main(int argc, char *argv[]) {
	int con = 0;
	char *hostname=NULL;
  struct sockaddr_in server;

	if(argc <= 1) {fprintf(stderr, "USAGE: sudo %s [-t count] <hostname>\n", argv[0]); return -1;}
	argParser(argc, argv, &con, &hostname);
	ping_count = con;

	if(lookupDNS(hostname, &server) == NULL) {perror("Error\n"); fprintf(stderr, "DNS lookup failed.\n");return -1;}

	fprintf(stdout, "PING: %s (%s): %d bytes of data.\n", hostname, lookupDNS(hostname, &server), PING_BYTES);

  if(socket(AF_INET, SOCK_RAW, IPPROTO_ICMP) <= -1) {perror("Error\n"); fprintf(stderr, "Cannot create socket\n"); return -1;}

  signal(SIGINT, handle_interrupt);
	ping(socket(AF_INET, SOCK_RAW, IPPROTO_ICMP), &server, lookupDNS(hostname, &server), hostname);

	return 0;
}
