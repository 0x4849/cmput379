/* **************************************************
   # Author: Brad Harrison                             *
   # Lecture: B1         				   			   *
   # Class : CMPUT 379                                 *
   # Lab TA: Mohomed Shazan Mohomed Jabbar             *
   #                                                   *
   # Lecturer: Dr. Mohammad Bhuiyan                    *
   # Created: March 18th, 2015                         *
   #***************************************************/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ctype.h>

#include <signal.h>
#include <time.h>

#define BUFLEN 512
#define IP 2130706433  /* 127.0.0.1 */

int netAtoNetB, netAtoNetC, netBtoNetA, netBtoNetC, netCtoNetA, netCtoNetB, invalidDestination;
char *fileName;

/* Generate random C code based upon http://faq.cprogramming.com/cgi-bin/smartfaq.cgi?answer=1042005782&id=1043284385 */
int getRandomValue(int minValue, int maxValue)
{
  static int Init = 0;
  int randomValue;
  
  if (Init == 0)
  {
    srand(time(NULL));
    Init = 1;
  }
  randomValue = (rand() % (maxValue - minValue + minValue) + minValue);
  
  return (randomValue);
}

/* This function takes in a source IP and a destination IP -- both randomly generated.
   From here, the packet generator updates its counters accordingly */

void updateIPCounters(int sourceIP, int destIP)
{
  int routerBSource, routerASource, routerCSource, routerBDest, routerADest, routerCDest;

  routerBSource = 0;
  routerASource = 0;
  routerCSource = 0;

  routerBDest = 0;
  routerADest = 0;
  routerCDest = 0;

  /* Store some flags to figure out who the source is and who the destination is */
  if (sourceIP == 1)
  {
    routerBSource = 1;
  }
  else if (sourceIP == 2)
  {
    routerASource = 1;
  }
  else if (sourceIP == 3)
  {
    routerCSource = 1;
  }

  if (destIP == 1)
  {
    routerBDest = 1;
  }
  else if (destIP == 2)
  {
    routerADest = 1;
  }
  else if (destIP == 3)
  {
    routerCDest = 1;
  }
  else if (destIP == 4)
  {
    invalidDestination += 1;
    return;
  }

  /* Now that we know who the source and destination are, update the counters through global variables accordingly */
  if (routerBSource && routerADest)
  {
    netBtoNetA += 1;
  }
  else if (routerBSource && routerCDest)
  {
    netBtoNetC += 1;
  }
  else if (routerASource && routerBDest)
  {
    netAtoNetB += 1;
  }
  else if (routerASource && routerCDest)
  {
    netAtoNetC += 1;
  }
  else if (routerCSource && routerBDest)
  {
    netCtoNetB += 1;
  }
  else if (routerCSource && routerADest)
  {
    netCtoNetA += 1;
  }
}

/* Convert string to int. I do not really handle the various possible error cases besides overflow. */
int convertStringToInt(char *stringCon)
{
  char *temp;
  long valueToTestNow;

  valueToTestNow = strtol(stringCon, &temp, 10);

  if(errno != 0)
  {
    fprintf(stderr, "Overflowed converting string to long\n");
    exit(0);
    
  }
  else if (*temp)
  {
   
  }
  else
  {
    //successful..
  }
  return (int) valueToTestNow;
}

/* Simply update the packet stats file by writing the global counters to the given file */
void updateStatsFile(void)
{

  FILE *f = fopen(fileName, "w");
  if (f == NULL)
  {
    printf("Error opening the packets file for updating.\nContinuing forward\n");
    return;
  }

  fprintf(f, "NetA to NetB: %i\n", netAtoNetB);
  fprintf(f, "NetA to NetC  %i\n", netAtoNetC);
  
  fprintf(f, "NetB to NetA  %i\n", netBtoNetA);
  fprintf(f, "NetB to NetC  %i\n", netBtoNetC);
  
  fprintf(f, "NetC to NetA  %i\n", netCtoNetA);
  fprintf(f, "NetC to NetB  %i\n", netCtoNetB);

  fprintf(f, "Invalid Destination %i\n", invalidDestination);
  fprintf(f, "Total Number %i\n", netAtoNetB+netAtoNetC+netBtoNetA+netBtoNetC+netCtoNetA+netCtoNetB+invalidDestination);
  
  fclose(f);
}

/* This program gathers some random integer values by callin getRandomValue.
   It uses these random values to determine which ipList to take an ip from.
   There is an equal probability of getting an unknown IP as there is getting a known IP.
   In addition, the payload is randomly chosen as well through getRandomValue */

char ** getRandomIP(const char *ipList1[11], const char *ipList2[11], const char *ipList3[11], const char *unknownIP, const char *payLoad[11])
{

  /* Allocate enough bytes to store an array.
     It is not completely an arrayOfIPs though:
     index 0 refers to the source IP of the packet to be generated
     index 1 refers to the destination IP of the packet to be generated
     index 2 refers to the payload of the packet to be generated */
  
  char ** arrayOfIPs = ( char ** ) malloc (sizeof (char *) * 3);
  
  int IPindex1, IPindex2, randomIndex, payLoadIndex, strLen1, strLen2, strLen3;
  char sourceIP[16], destIP[16];
  char payLoadString[20];
  
  IPindex1 = 0;
  IPindex2 = 0;

  /* Keep generating a new random number until
     the source array number  != destination array number */
  while (IPindex1 == IPindex2)
  {
    IPindex1 = getRandomValue(1,3);
    IPindex2 = getRandomValue(1,4);
  }

  /* Now that we know which source and destination arrays
     to gather IPs from, it is time to select a random
     index, which will get an IP from both source and
     destination arrays. */
  
  randomIndex = getRandomValue(0,10);

  /* If the random value selected is for RouterB, then
     copy a random IP into the source */
  
  if (IPindex1 == 1)
  {
    strcpy(sourceIP, ipList1[randomIndex]);
     
  }
  
  /* random value is for RouterA, then copy
     random IP into source */
  else if (IPindex1 == 2)
  {
    strcpy(sourceIP, ipList2[randomIndex]);
  }

  /* random value is for RouterC, so copy
     random IP into source */
  else if (IPindex1 == 3)
  {
    strcpy(sourceIP,ipList3[randomIndex]);
  }

  /* This says : if the ip selected is from one of the following
     network IP arrays : B, A, or C, then copy this into the
     destination IP.
     We handle random value == 4 separately because this is the
     destination : Unknown IP */
  
  if ((IPindex2 == 1) || (IPindex2 == 2) || (IPindex2 == 3))
  {
    randomIndex = getRandomValue(0,10);
    if (IPindex2 == 1)
    {
      strcpy( destIP, ipList1[randomIndex]);
    }
    else if (IPindex2 == 2)
    {
      strcpy( destIP, ipList2[randomIndex]);
    }
    else if (IPindex2 == 3)
    {
      strcpy(destIP, ipList3[randomIndex]);
    }
  }
  else if (IPindex2 == 4)
  {
    strcpy(destIP, unknownIP);
  }

  /* Get a random value to be used as the payload array index */
  payLoadIndex = getRandomValue(0,10);
  /* Copy a payload string into the payload array */
  strcpy(payLoadString, payLoad[payLoadIndex]);
 
 
  strLen1 = strlen(sourceIP);
  strLen2 = strlen(destIP);
  strLen3 = strlen(payLoadString);
 
  arrayOfIPs[0] = (char *) malloc(sizeof(char)*(strLen1+1));
  arrayOfIPs[1] = (char *) malloc(sizeof(char)*(strLen2+1));
  arrayOfIPs[2] = (char *) malloc(sizeof(char)*(strLen3+1));

  /* Copy all of the relevant data into "arrayOfIPs" */
  arrayOfIPs[0] = sourceIP;
  arrayOfIPs[1] = destIP;
  arrayOfIPs[2] = payLoadString;

  /* Now that we know who the source and destination are,
     it's time to update the counters that say who the
     packet was sent to/from */
  
  updateIPCounters(IPindex1, IPindex2);
  return arrayOfIPs; 
}

/* This signal handler handles the ctrl + c operation
   by having the packet generator update its statistics
   file and then it exits program execution */

void signalHandler(int signum)
{
  if (signum == SIGINT)
  {
    updateStatsFile();
    exit(0);
  }
}

void createStatsFile(void)
{
  FILE *f = fopen(fileName, "w");
  if (f == NULL)
  {
    perror("Error opening the packets statistics file for writing\nExiting now.");
    exit(0);
  }
}

int main( int argc, char ** argv)
{
  if ( argc != 3 )
  {
    printf("\n\nusage: %s <port number to connect to router>  <packets file path>\n\n", argv[0]);
    return 3;
  }


  int ttl, portnum, packetID;
  /* This array is used to store the generated source, destination, and payload */
  char **arrayOfParams;

  /* Here, ipList1 --> RouterB */
  const char *ipList1[11] = {"192.168.192.0", "192.168.192.1", "192.168.192.2", "192.168.192.3", "192.168.192.4", "192.168.192.5", "192.168.192.6", "192.168.192.7", "192.168.192.8", "192.168.192.9", "192.168.192.10"};

  /* ipList2 --> RouterA */
  const char *ipList2[11] = {"192.168.128.0", "192.168.128.1", "192.168.128.2", "192.168.128.3", "192.168.128.4", "192.168.128.5", "192.168.128.6", "192.168.128.7", "192.168.128.8", "192.168.128.9", "192.168.128.10"};

  /* ipList3 --> RouterC */
  const char *ipList3[11] = {"192.224.0.0", "192.224.0.1", "192.224.0.2", "192.224.0.3", "192.224.0.4", "192.224.0.5", "129.224.0.6", "192.224.0.7", "192.224.0.8", "192.224.0.9", "192.224.0.10"};
 
  const char *unknownIp[1];
  unknownIp[0]= "168.130.192.01";
  
  const char *payLoad[11] = {"Hello", "Hello World", "Candy", "Candy Apples", "RandomPayload1", "RandomPayload2", "RandomPayload3", "RandomPayload4", "RandomPayload5", "RandomPayload6", "RandomPayload7"};


  /* Declare buffers which will store the IP addresses and the payload
     once they have been randomly selected for packet generation. */
  
  char theSourceIP[16];
  char theDestinationIP[16];
  char thePayLoad[20];

  struct sigaction sa;
  struct sockaddr_in si_master, si_from;
  int sock, fromLength;

  char buf[BUFLEN];
  
  /* Set up the signal handler for CTRL + C SIGINT */
  sa.sa_handler = &signalHandler;
  sa.sa_flags = SA_RESTART;
  sigfillset(&sa.sa_mask);

  if (sigaction(SIGINT, &sa, NULL) == -1)
  {
    fprintf(stderr, "Could not handle SIGINT.\n");
  }
  
  fileName = argv[2];
  createStatsFile();
  
  /* initialize the global counters */
  netAtoNetB = 0;
  netAtoNetC = 0;
  netBtoNetA = 0;
  netBtoNetC = 0;
  netCtoNetA = 0;
  netCtoNetB = 0;
  invalidDestination = 0;

  /* Set up sockaddr variables */

  fromLength =sizeof(si_from);
  portnum = convertStringToInt(argv[1]);
        
  if ( ( sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP) ) == -1 )
  {
    printf("Error in creating socket");
    return 1;
  }

  /* Set up the socket intialization for the internal host */
  memset((char *) &si_master, 0, sizeof(si_master));
  si_master.sin_family = AF_INET;
  si_master.sin_port = htons(0);
  si_master.sin_addr.s_addr = htonl(IP);

  /* Set up the socket initialization for the router */
  memset((char *) &si_from, 0, sizeof(si_from));
  si_from.sin_family = AF_INET;
  si_from.sin_port = htons(portnum);
  si_from.sin_addr.s_addr = htonl(IP); 

  
  if ( bind(sock, (struct sockaddr *) &si_master, sizeof(si_master)) == -1 )
  {
    printf("Error in binding the socket");
    return 2;
  }

  printf("Starting to generate packets.\nCTRL+C packetsfile first to get in synch data\n");
        
  for (packetID=1;  ; packetID++)
  {

    /* arrayOfParams now contains a randomly generated source IP, destination IP, and payload */
    arrayOfParams = getRandomIP(ipList1, ipList2, ipList3, unknownIp[0],payLoad);
    /* Copy the relevant parameters into their own variables */
    strcpy(theSourceIP, arrayOfParams[0]);
    strcpy(theDestinationIP, arrayOfParams[1]);
    strcpy(thePayLoad, arrayOfParams[2]);
    /* Get TTL separately by randomly selecting a value between 1 and 4 */
    ttl = getRandomValue(1,4);

    /* Put the string of the packet ot be sent inside of buf */
    sprintf(buf,"%d, %s, %s, %d, %s", packetID, theSourceIP, theDestinationIP, ttl, thePayLoad);
    if (sendto(sock, buf, strlen(buf) + 1, 0, (struct sockaddr *) &si_from, fromLength)\
        == -1){
      perror("sendto encountered an error -- possibly due to inaccessible port number on the internal host.");

    }

    /* If the packet generated is of a multiple of 20, then update the stats file */
    if (packetID %20 == 0)
    {
      updateStatsFile();
    }
    if (packetID %2 == 0)
    {
      sleep(1);
    }
  }
  close(sock);
  return 0;
}
