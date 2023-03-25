#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netdb.h>
#include "Practical.h"

struct thread_info {    /* Used as argument to thread_start() */
    pthread_t thread_id;        /* ID returned by pthread_create() */
    int       thread_num;       /* Application-defined thread # */
    char     *server;
    char *servPort;
    float interval;
    int count;
    int flag;
    int size;
    char *echoString;
    int sock;
};



int main(int argc, char *argv[]) {



  //char *server = argv[1];     // First arg: server address/name
  //char *echoString = argv[2]; // Second arg: word to echo
  char *server;
  int serverReturn;
  int opt;
  char *servPort="3333";
  int printFlag=1;
  int count=20;
  int size=300;
  float interval=1.0;
  pthread_attr_t attr;
  void *res;

  struct thread_info *tinfo;

  while((opt = getopt(argc, argv, "S:c:i:p:s:n:")) != -1) { 
    switch(opt) { 

        //Server mode
        case 'S': 

            //Next input is the port number
            serverReturn=runServer(optarg);
            break;

        //Packet Count    
        case 'c': 

            //Get next input for packet count
            tinfo->count=atoi(optarg);
            fprintf(stderr,"Count: %d\n",count);

            break;

        //Ping interval
        case 'i': 

            // Use strtok since interval could be a decimal
            //Get number before decimal
            printf("Interval: \n");
            char *str = strtok(optarg, " . "); 
            int wholeNum=atoi(str);

            //get number after decimal
            str=strtok(optarg, " . ");
            int decNum=atoi(str);
            decNum=decNum/10;

            //THIS IS THE WRONG WAY TO BUILD A DECIMAL
            tinfo->interval=wholeNum+decNum;
            fprintf(stderr,"Interval: %f\n",interval);

            break; 

        //port number
        case 'p': 

            //Next input is the port number
            servPort=optarg;
            fprintf(stderr,"Port Number: %s\n",servPort);

            break;

        //size    
        case 's': 

            //Get next input for size
            tinfo->size=atoi(optarg);
            fprintf(stderr,"size: %d\n",size);
            break;

        //Don't print
        case 'n': 

            tinfo->flag=0;
            break;
 
        } 
    }
    server="babbage1";
    fprintf(stderr,"Server_IP: %s\n",server);


    // Tell the system what kind(s) of address info we want
  struct addrinfo addrCriteria;                   // Criteria for address match
  memset(&addrCriteria, 0, sizeof(addrCriteria)); // Zero out structure
  addrCriteria.ai_family = AF_UNSPEC;             // Any address family
  // For the following fields, a zero value means "don't care"
  addrCriteria.ai_socktype = SOCK_DGRAM;          // Only datagram sockets
  addrCriteria.ai_protocol = IPPROTO_UDP;         // Only UDP protocol
  fprintf(stderr,"Server_IP: %s\n Server Port: %s\n",server, servPort);

  // Get address(es)
  struct addrinfo *servAddr; // List of server addresses
  int rtnVal = getaddrinfo(server, servPort, &addrCriteria, &servAddr);
  if (rtnVal != 0)
    DieWithUserMessage("getaddrinfo() failed", gai_strerror(rtnVal));

  // Create a datagram/UDP socket
  fprintf(stderr,"Server_IP: %s\n",server);
  int sock = socket(servAddr->ai_family, servAddr->ai_socktype,
      servAddr->ai_protocol); // Socket descriptor for client
  if (sock < 0)
    DieWithSystemMessage("socket() failed");

  tinfo = calloc(2, sizeof(struct thread_info));

  int s = pthread_attr_init(&attr);

  for (int i = 0; i < 2; i++) {

  s = pthread_create(&tinfo[i].thread_id, &attr,
                           &sender, &tinfo[i]);

  s = pthread_create(&tinfo[i].thread_id, &attr,
                           &receive, &tinfo[i]);
  }

  for (int i = 0; i < 2; i++) {

  s = pthread_join(&tinfo[i].thread_id, &res);

  }
    
  free(res);
  free(tinfo);
    



    



  // Receive a response

  exit(0);
}

void * sender(void *args) {

  struct thread_info *tinfo = args;
  tinfo->echoString="AAAAA";
  size_t echoStringLen = strlen(tinfo->echoString);

  // Third arg (optional): server port/service
  //char *servPort = (argc == 4) ? argv[3] : "echo";

  // Tell the system what kind(s) of address info we want
  struct addrinfo addrCriteria;                   // Criteria for address match
  memset(&addrCriteria, 0, sizeof(addrCriteria)); // Zero out structure
  addrCriteria.ai_family = AF_UNSPEC;             // Any address family
  // For the following fields, a zero value means "don't care"
  addrCriteria.ai_socktype = SOCK_DGRAM;          // Only datagram sockets
  addrCriteria.ai_protocol = IPPROTO_UDP;         // Only UDP protocol

  // Get address(es)
  struct addrinfo *servAddr; // List of server addresses
  int rtnVal = getaddrinfo(tinfo->server, tinfo->servPort, &addrCriteria, &servAddr);
  if (rtnVal != 0)
    DieWithUserMessage("getaddrinfo() failed", gai_strerror(rtnVal));

  // Create a datagram/UDP socket
  // int sock; // socket descriptor
  if ((tinfo->sock = socket(AF_UNSPEC, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
      perror("socket() failed");
      exit(-1);
  }

  // Send the string to the server
  ssize_t numBytes = sendto(tinfo->sock, tinfo->echoString, echoStringLen, 0,
      servAddr->ai_addr, servAddr->ai_addrlen);
  if (numBytes < 0)
    DieWithSystemMessage("sendto() failed");
  else if (numBytes != echoStringLen)
    DieWithUserMessage("sendto() error", "sent unexpected number of bytes");

}




void *receive(void *args) {

  struct thread_info *tinfo = args;

  struct addrinfo addrCriteria;                   // Criteria for address match
  memset(&addrCriteria, 0, sizeof(addrCriteria)); // Zero out structure
  addrCriteria.ai_family = AF_UNSPEC;             // Any address family
  // For the following fields, a zero value means "don't care"
  addrCriteria.ai_socktype = SOCK_DGRAM;          // Only datagram sockets
  addrCriteria.ai_protocol = IPPROTO_UDP;         // Only UDP protocol

  // Get address(es)
  struct addrinfo *servAddr; // List of server addresses
  int rtnVal = getaddrinfo(tinfo->server, tinfo->servPort, &addrCriteria, &servAddr);
  if (rtnVal != 0)
    DieWithUserMessage("getaddrinfo() failed", gai_strerror(rtnVal));


  struct sockaddr_storage fromAddr; // Source address of server
  // Set length of from address structure (in-out parameter)
  socklen_t fromAddrLen = sizeof(fromAddr);
  char buffer[MAXSTRINGLENGTH + 1]; // I/O buffer
  ssize_t numBytes = recvfrom(tinfo->sock, buffer, MAXSTRINGLENGTH, 0,
      (struct sockaddr *) &fromAddr, &fromAddrLen);
  if (numBytes < 0)
    DieWithSystemMessage("recvfrom() failed");
  else if (numBytes != tinfo->size)
    DieWithUserMessage("recvfrom() error", "received unexpected number of bytes");

  // Verify reception from expected source
  if (!SockAddrsEqual(servAddr->ai_addr, (struct sockaddr *) &fromAddr))
    DieWithUserMessage("recvfrom()", "received a packet from unknown source");

  freeaddrinfo(servAddr);


  //close(sock);

}