#include <arpa/inet.h>
#include <limits.h>
#include <math.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>


int is_server = 0;
int packet_count = 10;
double interval;
int port = 33333;
int size = 200;
int no_print = 0;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

char* getFinalArg() {
    extern int optind;
    extern char **environ;

    int argc = 0;
    char **argv = environ;

    // count the number of arguments
    while (argv[argc] != NULL) {
        argc++;
    }

    if (argc > 1) {
        return argv[argc-1];
    } else {
        return NULL;
    }
}

void run_server() {
  printf("Starting UDP ping server (port=%d)\n", port);

  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in serv_addr = {0};
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(port);

  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in cli_addr = {0};
  socklen_t cli_addr_len = sizeof(cli_addr);

  char buffer[size];

  while (1) {
    int n = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                     (struct sockaddr *)&cli_addr, &cli_addr_len);
    if (n < 0) {
      perror("recvfrom");
      exit(EXIT_FAILURE);
    }

    n = sendto(sockfd, buffer, n, 0, (struct sockaddr *)&cli_addr,
               cli_addr_len);
    if (n < 0) {
      perror("sendto");
      exit(EXIT_FAILURE);
    }
    if (!no_print)
      printf("Received %d bytes from %s\n", n, inet_ntoa(cli_addr.sin_addr));
  }
  close(sockfd);
}

void *send_ping(void *arg) {
  //char *ip_addr = (char *) arg;

  int sockfd = *((int *)arg);

  int seq = 1;
  double rtt;
  int packets_sent = 0, packets_received = 0;
  double min_rtt = INT_MAX, max_rtt = 0, avg_rtt = 0;

  struct sockaddr_in serv_addr = {0};
  serv_addr.sin_family = AF_INET;


  // serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  serv_addr.sin_addr.s_addr = inet_addr(getFinalArg());
  serv_addr.sin_port = htons(port);

  struct timespec start_time;
  clock_gettime(CLOCK_REALTIME, &start_time);

  double totalTime;

  while (packet_count == 0 || seq <= packet_count) {
    char buffer[size];
    struct timespec send_time, recv_time;
    // Prepare ping packet
    memset(buffer, 0, size);
    snprintf(buffer, size, "PING %d", seq);
    // Calculate proper wait time until next ping
    struct timespec wait_time;
    wait_time.tv_sec = start_time.tv_sec + (seq - 1) * interval;
    wait_time.tv_nsec = start_time.tv_nsec;
    while (wait_time.tv_nsec >= 1000000000L) {
      wait_time.tv_nsec -= 1000000000L;
      wait_time.tv_sec++;
    }

    pthread_mutex_lock(&lock);
    // Wait for proper time to send next ping
    pthread_cond_timedwait(&cond, &lock, &wait_time);
    pthread_mutex_unlock(&lock);

    // Send ping packet
    clock_gettime(CLOCK_REALTIME, &send_time);
    int n = sendto(sockfd, buffer, size, 0, (struct sockaddr *)&serv_addr,
                   sizeof(serv_addr));
    if (n < 0) {
      perror("sendto");
      exit(EXIT_FAILURE);
    }
    packets_sent++;

    // Receive pong packet
    char recv_buffer[size];
    socklen_t serv_addr_len = sizeof(serv_addr);
    n = recvfrom(sockfd, recv_buffer, size, 0, (struct sockaddr *)&serv_addr,
                 &serv_addr_len);
    if (n < 0) {
      perror("recvfrom");
      exit(EXIT_FAILURE);
    }
    packets_received++;

    // Get receive time and calculate round-trip time
    clock_gettime(CLOCK_REALTIME, &recv_time);
    rtt = (recv_time.tv_sec - send_time.tv_sec) * 1000.0 +
          (recv_time.tv_nsec - send_time.tv_nsec) / 1000000.0;

    totalTime += rtt;

    // Update min/max/avg RTT
    if (rtt < min_rtt) {
      min_rtt = rtt;
    }
    if (rtt > max_rtt) {
      max_rtt = rtt;
    }
    avg_rtt += rtt;

    // Print ping-pong message
    if (!no_print){
      printf("%d %d %.3f\n", seq, n, rtt);
    }
    else {
      printf("*");
    }
    seq++;
  }

  // Print summary
  double packet_loss = 100.0 * (packets_sent - packets_received) / packets_sent;
  double avg_rtt_ms = avg_rtt / packets_received;
  printf(
      "\n%d packets transmitted, %d received, %.1f%% packet loss, time %.3f ms\n",
      packets_sent, packets_received, packet_loss, totalTime);
  printf("rtt min/avg/max = %.3f/%.3f/%.3f msec\n", min_rtt, avg_rtt_ms,
         max_rtt);
  exit(EXIT_SUCCESS);
}

void run_client() {
  printf("Starting UDP ping client (server=localhost:%d, packet_count=%d, "
         "interval=%fms, size=%d)\n",
         port, packet_count, interval, size);

  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  pthread_t tid;
  pthread_create(&tid, NULL, send_ping, &sockfd);

  while (1) {
    // wait for send_ping to complete packet_count number of pings
    pthread_mutex_lock(&lock);
    while (packet_count == 0 || packet_count != -1) {
      pthread_cond_wait(&cond, &lock);
    }
    pthread_mutex_unlock(&lock);
    pthread_cancel(tid);
    break;
  }
  close(sockfd);
}

int main(int argc, char *argv[]) {
  int opt = 0;
  is_server = 0;
  char *server_ip_address = NULL;

  while ((opt = getopt(argc, argv, "c:i:p:s:nS")) != -1) {
    switch (opt) {
    case 'c':
      packet_count = atoi(optarg);
      break;
    case 'i':
      interval = atof(optarg);
      break;
    case 'p':
      port = atoi(optarg);
      break;
    case 's':
      size = atoi(optarg);
      break;
    case 'n':
      no_print = 1;
      break;
    case 'S':
      is_server = 1;
      break;
    default:
      fprintf(stderr,
              "Usage: %s [-c count] [-i interval] [-p port] [-s size] [-n] "
              "[-S] server_ip_address\n",
              argv[0]);
      exit(EXIT_FAILURE);
    }
  }

  if (optind < argc) {
    server_ip_address = argv[optind];
  }

  fprintf(stderr, "Count %d\n", packet_count);
  fprintf(stderr, "Size %d\n", size);
  fprintf(stderr, "Interval %.3f\n", interval);
  fprintf(stderr, "Port %d\n", port);
  fprintf(stderr, "Server_ip %s\n", server_ip_address ? server_ip_address : "");

  if (is_server) {
    run_server();
  } else {
    run_client();
  }
  return 0;
}