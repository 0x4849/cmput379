/* **************************************************
   # Author: Brad Harrison         	        		   *
   # Lecture: B1         				  			   *
   # Class : CMPUT 379                                 *
   # Lab TA: Mohomed Shazan Mohomed Jabbar             *
   #                                                   *
   # Lecturer: Dr. Mohammad Bhuiyan                    *
   # Created: March 18th, 2015                         *
   #***************************************************/

/* This file manually converts between an IP address string
   and basically a numerical value to represent it.
   This is the reason the file has so many lines of code --
   because I did not use a library function for this conversion
   since I did not realize that they existed at the time */

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
#include <sys/types.h>
#include <sys/wait.h>
#include <err.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/file.h>
#include <sys/stat.h>


typedef unsigned char uint1;

#define ROUTERBUFFERSIZE 3000
#define AMASK 0xffff8000
#define BMASK 0xffffc000 
#define CMASK 0xffff0000

int unroutablePackets, expiredPackets, deliveredDirectPackets, routerBPackets, routerCPackets;

/* Declare a bunch of net masks -- here we declare [0..n] worth of net masks, i.e. all of the possible net masks for the ipv4 */
unsigned int netMask[33] = {0x00000000,0x80000000,0xc0000000, 0xe0000000, 0xf0000000, 0xf8000000,0xfc000000, 0xfe000000, 0xff000000, 0xff800000, 0xffc00000, 0xffe00000,0xfff00000, 0xfff80000, 0xfffc0000, 0xfffe0000, 0xffff0000, 0xffff8000,0xffffc000,0xffffe000,0xfffff000,0xfffff800,0xfffffc00,0xfffffe00,0xffffff00,0xffffff80,0xffffffc0,0xffffffe0,0xfffffff0,0xfffffff8,0xfffffffc,0xfffffffe,0xffffffff};

char *fileName;

/* Declare the specified parameters expected for the program */
static void usage()
{
  extern char * __progname;
  fprintf(stderr, "usage: %s portnumber routingtablefilepath statisticsfile\n", __progname);
  exit(1);
}

/* Update the stats file by printing the required variables. */
void updateStatsFile(void)
{
  FILE *f = fopen(fileName, "w");
  if (f == NULL)
  {
    perror("Error opening the file.\nContinuing with the function.\n");
    return;
  }

  fprintf(f, "expired packets: %i\n",expiredPackets);
  fprintf(f, "unroutable packets: %i\n", unroutablePackets);
  fprintf(f, "delivered direct: %i\n", deliveredDirectPackets);
  fprintf(f, "router B: %i\n", routerBPackets);
  fprintf(f, "router C: %i\n", routerCPackets);
  /* Uncomment this line for easy comparison with the other file */
  //fprintf(f, "Total Number %i\n", expiredPackets+unroutablePackets+deliveredDirectPackets+routerBPackets+routerCPackets);
  
  fclose(f);
}

/* Convert a string to integer. There is only one cavet : if the integer is interleaves with non numerical chararacters,
   then the non-numerical characters are filtered out of the string */

int convertStringToInt(char *stringCon)
{
  char *temp;
  long valueToTestNow;

  valueToTestNow = strtol(stringCon, &temp, 10);

  /* Other error values ignored. */
  if(errno != 0)
  {
    fprintf(stderr,"Error converting string.\n");
    //printf("Conversion error, %s\n", strerror(errno));
  }
  else if (*temp)
  {
    //printf("Assuming the  number is: %li, non-convertible part of port: %s\n", valueToTestNow, temp);
  }
  else
  {
    //printf("Converted successfully: The value to test now number is %li\n", valueToTestNow);
  }
  return (int) valueToTestNow;
}

/* Return a boolean value such that it is equal to 1 if the ip masked out from packet is equivalent to one that is expected.\n" */ 
int checkMatchedPrefix(int ipMasked[4], char formattedIpArray[4][4])
{
  return ((ipMasked[0] == convertStringToInt(formattedIpArray[0])) && (ipMasked[1] == convertStringToInt(formattedIpArray[1])) \
          && (ipMasked[2] == convertStringToInt(formattedIpArray[2])) && (ipMasked[3] == convertStringToInt(formattedIpArray[3])));
  
}

/* This function does extra work to convert the ip destination into an address we can handle by padding extra zeros */
void convertStringToMultiArray(char listOfParams[16], char ipDestination[4][4], int zeroPad[4])
{
  int decimalCounter = 0;
  int secondaryCounter = 0;
  int newI;
  int i;
  int isNew = 1;
  
  for ( i = 0; i <= 3; i++ )
  {
    if (zeroPad[i] == 1)
    {
      /* Since zeropad is 1, we need to pad the ip subsection with one zero to make it behave properly. */
      ipDestination[i][0] = '0';
    }
    else if (zeroPad[i] == 2)
    {
      /* Since zeroPad is 2, then we need to pad the ip subsection with two zeros to make it behave properly */
      ipDestination[i][0] = '0';
      ipDestination[i][1] = '0';
    }
  }

  for ( newI = 0; newI <= 14 && decimalCounter <= 3; newI++ )
  {
    if (isNew)
    {
      /* We have a new subsection starting number or after a decimal
         secondaryCounter just keeps track of whether or not it is still
         safe to add to our string(based on how many zeros should be padded.*/
      secondaryCounter = zeroPad[decimalCounter];
      isNew = 0;
    }
    if (listOfParams[newI] == '.')
    {
      /* We have encountered a decimal so we need to add a zero byte to one of the ip destination arrays.
         (The ip destination is built into three separate arrays) */
      ipDestination[decimalCounter][secondaryCounter] = '\0';
      decimalCounter += 1;
      /* Re-set the secondary counter since we encountered a decimal */
      secondaryCounter = 0;
      /* Set the new flag */
      isNew = 1;
    }
    else if (isdigit(listOfParams[newI]))
    {
      if (secondaryCounter+1 <= 3)
      {
        /* Add another integer value to the ipDestination array since it is safe to do so. */ 
        ipDestination[decimalCounter][secondaryCounter] = listOfParams[newI];
        secondaryCounter += 1;
      }
    }
    
  }
  /* We are finished, so put a zero byte in the end to make conversion functions work better. */
  ipDestination[decimalCounter][secondaryCounter] = '\0';
}

/* This function simply makes sure that each IP address has three numbers in before and after each decimal.
   It does this by padding zeros as necessary, so 192.1.4.2 --> 192.001.004.002, which makes binary operations
   such as masking all the easier.
*/
void zeroPadCounter(char listOfParams[16], int zeroPad[4])
{
  int i;
  int decimalCounter = 0;
  int numChars = 0;
  for (i = 0; i <= 14; i++)
  {
    /* If we encounter a decimal, re-count how many numerical characters were encountered.
       Based upon the number of numerical characters encountered before decimal, it will
       determine how many zeros are to be padded. */
    if (listOfParams[i] == '.')
    {
      if (numChars == 1)
      {
        /* The number of numerical characters encountered before the decimal was one,
           so we need to pad it with two zeros. */
        zeroPad[decimalCounter] = 2;
      }
      else if (numChars == 2)
      {
        /* The number of numerical characters encountered before the decimal was two,
           so we need to pad it with one zero. */
        zeroPad[decimalCounter] = 1;
      }
      else if (numChars == 3)
      {
        /* The number of numerical characters encountered before the decimal was three,
           so we need to pad it with one zero. */
        zeroPad[decimalCounter] = 0;
      }
      
      decimalCounter += 1;
      numChars = 0;
    }
    else if (isdigit(listOfParams[i]))
    {
      /* We encountered a numerical character, so increment the numerical character counter numChars by 1 */
      numChars += 1;

    }
  }

  /* Now that we are done padding zeros for every value before a decimal point, we need to pad zeros for the
     last three numerical values(since we did not encounter a decimal point in order to update the last three
     values.*/
  
  if (numChars == 1)
  {
    zeroPad[3] = 2;
  }
  else if (numChars == 2)
  {
    zeroPad[3] = 1;
  }
  else if (numChars == 3)
  {
    zeroPad[3] = 0;
  }
}

/* This file parses the RT text file, but it does not do any error checking. */
void textParse (char *RTFile, char ipArray[3][16], char netPrefixLength[3][3], char hopDirection[3][8])
{  

  FILE *rtTable;
  /* Create some values for keeping track of characters during parsing */
  int lineIndex, i, j, spaceEncountered, newLineEnabled, c, newLineCounter;
  newLineCounter = 0;
  
  rtTable = fopen(RTFile, "r");
  /* Opening RT File Error. */
  if (rtTable == 0)
  {
    perror("Cannot open input RT file because it doesn't exist.\nExiting now\n");
    exit(-1);
  }
  else
  {
    lineIndex = 0;
    i = 0;
    j = 0;
    spaceEncountered = 0;
    newLineEnabled = 0;
    while (( c = fgetc(rtTable)) != EOF )
    {
      if (c == '\n')
      {
        /* Encountered a new line which can only happen after reading in the hop direction, so this means that the next values
           will be from the next IP Address of the network */
        newLineCounter +=1;
        /* newLineEnabled makes sure that we haven't encountered a duplicate new line character
           before encountering a new value because this can be ignored. */
        if (!newLineEnabled)
        {
          /* The only time we should encounter a new line is after i = 2 or after parsing in the hop direction,
             so check this value and then add a zero byte to the end of hop direction. */
          if (i == 2)
          {
            hopDirection[lineIndex][j] = '\0';
          }

        }
        /* Re-set space encountered value */
        spaceEncountered = 0;
        /* Enable newLineEnabled to indicate that a new line has been seen already before any alphanumerical values */
        newLineEnabled = 1;

      }
      else if ((c == ' ' || c == '\t') && !newLineEnabled)
      {
        /* If we encounter a space or a tab then we know that we have a new value and should ignore future spaces until
           an alphanumerical value is present. */
        spaceEncountered = 1;
      }

      /* We've encountered a new line and finally we've encountered an alphanumerical character.
         Should be in i = 0 or the ip address of the network. */
      else if (newLineEnabled && isalnum(c) && !spaceEncountered)
      {

        /* line index takes into account which network we are looking at :
           lineindex = 0 is the first network(line 1 of RT), line = 1 is the second network, etc */
        lineIndex += 1;
        /* Reset everything back to 0 since we have encountered a new value */
        spaceEncountered = 0;
        i = 0;
        j = 0;


        /* The reason we add the character to ipArray is because we can only be inside
           of the ip address if we encountered a new line character. */
        ipArray[lineIndex][j] = c;

        /* Re-set new line enabled back to false because we have encountered a new value. */
        newLineEnabled = 0;

        /* Increment j which is essentially the character pointer.
           string[i][j] --> i is one of three values in a line. j is its character index */
        j += 1;
      }
      /* Encountered a space and now we've finally encountered a new line character :
         this means that we've likely either reached i = 1 (net prefix length) or i = 2(hop direction) */
      else if (spaceEncountered && isalnum(c) && !newLineEnabled)
      {
        /* if i = 0 right now, then it means that we've reached i = 1(net prefix length) of the line.
           So this means that we were inside of the IP, finished parsing it, and then the space designated
           the end of this IP string */
        if (i == 0)
        {
          /* store the zero byte into the end of the IP */
          ipArray[lineIndex][j] = '\0';
        }
        else if (i == 1)
        {
          /* i was equal to 1 before hitting a space so this means that we've encountered i = 2 or the hop direction */
          netPrefixLength[lineIndex][j] = '\0';
        }
        else if (i == 2)
        {
          /* It is possible that the hop direction has a space after it, so this needs to be accounted for. */
          hopDirection[lineIndex][j] = '\0';
        }

        /* Re-set space encountered because we have reached a new alphanumerical value */
        spaceEncountered = 0;
        /* Increment i to indicate that we are in the next string or value in the current line */
        i += 1;
        /* Re-set the character pointer to 0 for the next string */
        j = 0;
        
        if (i == 1)
        {
          /* If i is equal to 1 after incrementing(and reaching a space),
             then we need to store the current character in netprefixlength */
          netPrefixLength[lineIndex][j] = c;
        }
        else if (i == 2)
        {
          /* i is 2, so it means we've reached hop direction after previously
             being inside of net prefix length and then hitting a space */
          hopDirection[lineIndex][j] = c;
        }

        /* Increment the character pointer for the new value.*/
        j += 1;
      }
      else if (isalnum(c) || c == '.')
      {
        /* If we have encountered a decimal, then just like a space or a new line character
           it means that we are about to encounter new value */
        if (i == 0)
        {
          /* add the decimal point to the end of the current ip value if we see a decimal */
          ipArray[lineIndex][j] = c;
        }
        else if(i == 1)
        {
          /* Quite unlikely the net prefix length has decimals in it, but just in case */
          netPrefixLength[lineIndex][j] = c;
        }
        /* Another quite unlikely scenario */
        else if (i == 2)
        {
          hopDirection[lineIndex][j] = c;
        }
        /* Increment character pointer and keep reading values in */
        j += 1;
      } 
    }
  }
}

/* This function is called just to clarify if a certain packet's hop direction
   can be matched to a well known hop direction */
void matchHopDirection(char matchedDirection[], int packetID, char destIP[])
{
  char routerB[] = "RouterB";
  char routerC[] = "RouterC";
  
  if (strcmp(matchedDirection,routerB) == 0)
  {
    /* Router B and the packet's hop direction are a match,
       so increment routerBPackets */
    routerBPackets += 1;
  }

  else if (strcmp(matchedDirection, routerC) == 0)
  {
    /* Router C and the packet's hop direction are a match,
       so increment routerCPackets */
    routerCPackets += 1;
  }

  else if (convertStringToInt(matchedDirection) == 0)
  {
    /* Router A and the packet's hop direction are a match,
       so increment delivered direct packets */
    printf("Delivering direct: packetID=%i, dest=%s\n", packetID, destIP);
    deliveredDirectPackets += 1;
  }
}

/* checkValidity is the main function that checks the validity of a packet
   in terms of it being a match in the router table
   Manual conversions between string IP addresses and binary segments take place here.*/

void checkValidity(unsigned char *receiveMessage,\
                   char netPrefixLength[3][3], char hopDirection[3][8], char formattedIpArray1[4][4], \
                   char formattedIpArray2[4][4], char formattedIpArray3[4][4])
{
    
  int packetID, isMatched, ipMasked[4], zeroPad[4],\
      numberToTest1, numberToTest2, numberToTest3, numberToTest4,       \
      i, j, commaCounter, ttl, tempValueIndex, valueTester;
 
  
  char listOfParams[4][16];
  char ipDestination[4][4];
  
  i = 0;
  j = 1;

  /* receiveMessage is the packet's buffer or string. We should transfer this
     string into a more managable multi dimensional array that can store the various
     parameters of the string. */ 
  listOfParams[i][j-1] = *receiveMessage;
  commaCounter = 0;
  while (*receiveMessage++)
  {
    /* If we encounter a comma inside of the packet's string, then we need to
       write a 0 byte to the end of the current parameter to indicate its end */
    if (*receiveMessage == ',')
    {
      listOfParams[i][j] = '\0';
      /* Re-set the character pointer 'j' back to 0
         but increment the paramter/string pointer called 'i'. */
      j = 0;
      i += 1;
      /* Increment comma counter for later verifying
         that the packet is valid */
      commaCounter++;
    }

    else if ((isalnum(*receiveMessage)) || *receiveMessage == '.')
    {
      /* If the current message is alphanumerical or it contains a period,
         then add it to the current parameter and increment the character
         pointer */
      listOfParams[i][j] = *receiveMessage;
      j++;
    }
  }
  /* A packet should have four commas.
     If it does not have four commas, then return. */
  if (commaCounter != 4)
  {
    fprintf(stderr, "Error parsing the packet. Not enough commas.\nExiting now.");
    return;
  }

  ttl = convertStringToInt(listOfParams[3]);
  if (ttl - 1 <= 0)
  {
    /* The time to live has hit zero, so increment expiredPackets */
    expiredPackets += 1;
    return;
  }

  /* Pad the zeros of the incoming packet IP to get it
     to match up with the RT IP */
  zeroPadCounter(listOfParams[2], zeroPad);
  convertStringToMultiArray(listOfParams[2], ipDestination, zeroPad);

  /* Get the numerical values of the IP address.
     These numerical values do not contain decimals.
     a.b.c.d gets put into numberToTest1..4 */
  numberToTest1 = convertStringToInt(ipDestination[0]);
  numberToTest2 = convertStringToInt(ipDestination[1]);
  numberToTest3 = convertStringToInt(ipDestination[2]);
  numberToTest4 = convertStringToInt(ipDestination[3]);

  valueTester = 0;
  isMatched = 0;
  packetID = convertStringToInt(listOfParams[0]);

  /* valueTester is essentially an index that is used for
     the netPrefixLength. We have netPrefixLength[0..2],
     which corresponds to the three net prefix lengths of
     the three networks in the RT file.
     Using these net prefix lengths we can use masks
     and compare the resultant IP addresses with the RT
     IP addresses */
  
  while ((valueTester <= 2) && (!isMatched))
  {
    tempValueIndex = convertStringToInt(netPrefixLength[valueTester]);

    /* The net prefix lengths can supply a specific mask. Each mask
       from netMask is indexes from 0..32 and corresponds to the
       particular masks needed per the given net prefix length. */
    unsigned int valueToTest = netMask[tempValueIndex];
    ipMasked[0] = (((valueToTest & 0xFF000000) >> 24) & numberToTest1);
    ipMasked[1] = (((valueToTest & 0x00FF0000) >> 16) & numberToTest2);
    ipMasked[2] = (((valueToTest & 0x0000FF00) >> 8) & numberToTest3);
    ipMasked[3] = (((valueToTest & 0x000000FF) >> 0) & numberToTest4);

    /* checkMatchedPrefix is able to check if the ip masked out matches
       a given RT ip address which is contained within formattedIPArray1..3. */
    if ((valueTester == 0) && (checkMatchedPrefix(ipMasked, formattedIpArray1)))
    {
      matchHopDirection(hopDirection[0],packetID,listOfParams[2]);
      isMatched = 1;
    }
    
    else if ((valueTester == 1) && (checkMatchedPrefix(ipMasked, formattedIpArray2)))
    {
      matchHopDirection(hopDirection[1],packetID,listOfParams[2]);
      isMatched = 1;
    }

    else if ((valueTester == 2) && (checkMatchedPrefix(ipMasked, formattedIpArray3)))
    {
      matchHopDirection(hopDirection[2],packetID,listOfParams[2]);
      isMatched = 1;
    }

    /* Increment the index */
    valueTester += 1;
    
  }
  if (!isMatched)
  {
    /* If after testing all 3 network IP addresses and we do not get a match
       after masking out the destination IPs, then increment unroutablePackets. */
    unroutablePackets += 1;
  }
}

void signalHandler(int signum)
{
  /* Handle ctrl + c by updating stats file and exiting */
  if (signum == SIGINT)
  {
    updateStatsFile();
    exit(0);
  }
}

/* The following function validates the text file as best as it can.
   If the net prefix length isn't between 0 and 32, the hop direction isn't as specified,
   or if the ip address isn't valid, then we return an error. */
void validateTextFile(char ipArray[3][16], char netPrefixLength[3][3], char hopDirection[3][8])
{
 
  int tempValue;
  int i;
  char hopDirectionA[] = "0";
  char hopDirectionB[] = "RouterB";
  char hopDirectionC[] = "RouterC";

  i = 0;
  /* RT contains three lines of value, so we only need to test three lines */
  while (i <= 2)
  {
    /* There are three tests which are all performed at one time :
       Test 1 : inet_aton checks if the IP is a valid IP Address.
       Test 2 : The net prefix length is confirmed to be between 0 and 32
       Test 3 : The hop direction is confirmed to be one of the known hop directions */
    
    struct in_addr maskAddr;
    tempValue = convertStringToInt(netPrefixLength[i]);
    inet_aton(ipArray[i], &maskAddr);
    if ((inet_aton(ipArray[i], &maskAddr)!=0) &&  \
        (tempValue >= 0) && (tempValue <= 32) &&                        \
        ((strncmp(hopDirectionA, hopDirection[i], strlen(hopDirectionA)) == 0) ||
         (strncmp(hopDirectionB, hopDirection[i], strlen(hopDirectionB)) == 0) ||
         (strncmp(hopDirectionC, hopDirection[i], strlen(hopDirectionC)) == 0)))
    {
      i += 1;
    }
    else
    {
      fprintf(stderr, "Invalid RT File Detected.\nExiting now\n");
      usage();
    }
  }
}

void createStatsFile(void)
{
  FILE *f = fopen(fileName, "w");
  if (f == NULL)
  {
    perror("Error opening the statistics file for writing\nExiting now.");
    exit(0);
  }
}

/*Give main access to parameters passed from command shell.
  argc is number of words, argv array containing words.*/ 
int main ( int argc, char *argv[] )
{
  if (argc != 4)
  {
    printf("Invalid number of parameters.\n\nusage: %s <port number to listen to> <routing table file path> <statistics file path> \n\n", argv[0]);
    exit(0);
  }
  
  u_long p;
  u_short portnum;

  char *ep;
  int zeroPad1[4], zeroPad2[4], zeroPad3[4], packetCounter, receiveBytes,\
      sock;
  char *RTFile = argv[2];
  
  /* Pre-allocating '|' characters so that parsing the given arrays cannot contain
     random numbers that could possibly create problems when parsing */
  char ipArray[3][16] = {{"|||||||||||||||"},{"|||||||||||||||"},{"|||||||||||||||"}};
  char netPrefixLength[3][3] = {{"||"},{"||"}, {"||"}};
  char hopDirection[3][8] = {{"|||||||"},{"|||||||"},{"|||||||"}};

  unsigned char routerBuffer[ROUTERBUFFERSIZE];

  /* formattedIPArray is where we put the three IPs from the RT file after they
     are zero padded */
  char formattedIpArray1[4][4];
  char formattedIpArray2[4][4];
  char formattedIpArray3[4][4];
  
  struct sigaction sa;
  struct  sockaddr_in master, from;

  /* This port checking error code is from Bob Beck's lab TCP server demo code */
  errno = 0;
  p = strtoul(argv[1], &ep, 10);
  if (*argv[1] == '\0' || *ep != '\0')
  {
    /* parameter for the port wasnt a valid number */
    fprintf(stderr, "%s - not a number\n", argv[1]);
    usage();
  }
  if ((errno == ERANGE && p == ULONG_MAX) || (p > USHRT_MAX))
  {
    /* Port is a number but cannot fit into its buffer -- exit program 
     * 
     */
    fprintf(stderr, "%s - port value out of range\n", argv[1]);
    usage();
  }

  portnum = p;
  
  fileName = argv[3];
  createStatsFile();

  /* Parse the RT file and then check if the RT file should be valid or not */
  textParse(RTFile, ipArray, netPrefixLength, hopDirection);
  validateTextFile(ipArray, netPrefixLength, hopDirection);

  /* Set up the signal handler for ctrl + C actions */
  sa.sa_handler = &signalHandler;
  sa.sa_flags = SA_RESTART;
  sigfillset(&sa.sa_mask);

  if (sigaction(SIGINT, &sa, NULL) == -1)
  {
    fprintf(stderr, "Could not handle SIGINT.\n");
  }

  /* Set up the UDP socket in terms of internal and external or master and from
     bindings */
  socklen_t fromLength = sizeof(from);

  sock = socket (AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    perror ("Server: cannot open master socket");
    exit (1);
  }

  portnum = convertStringToInt(argv[1]);
  
  master.sin_family = AF_INET;
  master.sin_addr.s_addr = INADDR_ANY;
  master.sin_port = htons (portnum);

  if (bind (sock, (struct sockaddr*) &master, sizeof (master))) {
    perror ("Server: cannot bind master socket");
    exit (1);
  }



  /* Pad zeros for the three ip addresses(one per RT network).
     This is done for one reason : to ensure that incoming packets
     and the RT ip addresses are of an equivalent form with padded
     zeros as needed to reach three numerical characters before and
     after each decimal in an IP string. */
     
  zeroPadCounter(ipArray[0], zeroPad1);
  zeroPadCounter(ipArray[1], zeroPad2);
  zeroPadCounter(ipArray[2], zeroPad3);

  /* Store the padded zeros + IP address into formattedIPArray1..3 */
  convertStringToMultiArray(ipArray[0], formattedIpArray1,zeroPad1);
  convertStringToMultiArray(ipArray[1], formattedIpArray2,zeroPad2);
  convertStringToMultiArray(ipArray[2], formattedIpArray3,zeroPad3);
  
  packetCounter = 0;
  printf("Starting the router for listening for packets.\n");
  while (1)
  {
    receiveBytes = recvfrom(sock, routerBuffer, ROUTERBUFFERSIZE, 0, \
                            (struct sockaddr *)&from, &fromLength);
    if (receiveBytes > 0)
    {
      packetCounter += 1;
      routerBuffer[receiveBytes] = 0;
      checkValidity(routerBuffer, netPrefixLength, hopDirection, formattedIpArray1,\
                    formattedIpArray2, formattedIpArray3);

      if (packetCounter % 20 == 0)
      {
        /* We've received a packet that's a multiple of 20, so update the stats file */
        updateStatsFile();
      }
    }
    
    else if (receiveBytes <= 0)
    {
      fprintf(stderr, "An error occurred receiving an incoming packet.\nContinuing forwards");
      continue;
    }

  }
  

  return 0;
}
