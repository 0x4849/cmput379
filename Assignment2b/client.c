/* **************************************************
   # Author: Brad Harrison         				       *
   # Lecture: B1        							   *
   # Class : CMPUT 379                                 *
   # Lab TA: Mohomed Shazan Mohomed Jabbar             *
   #                                                   *
   # Lecturer: Dr. Mohammad Bhuiyan                    *
   # Created: March 18th, 2015                         *
   #***************************************************/

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <err.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <time.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#define BUFFERLENGTH 1024

static void usage()
{
  extern char * __progname;
  fprintf(stderr, "usage: %s ipnumber portnumber filename\n", __progname);
  exit(1);
}


/*
  # Function recvFromWithTimeOut
  #----------------------------------------------------------------------
  # Subroutine recvfromWithTimeOut:
  # 1 : Uses select() to see if any data is present on the socket(waits numSecs seconds)
  # If no data is present within numSecs, returns -1 for an error state.
  #
  # 2 : If data is present on the socket, then ioctl() checks to see how many bytes are present.
  # If 2 bytes are present, then the code checks to see if a $ string is present(to indicate 
  # end of string). If $ present --> return -55 to indicate this.
  # If 5 bytes are present, then the code checks to see if "fse@" string is present, which
  # the server may send to the client at some point to indicate a file send error.
  # If "fse@" is present --> return -98 to indicate this.
  #
  # 3 : If neither file send error or $ are present, then bytes are copied into the buffer
  # called "chunkList" and the number of bytes sucessfully read are returned to the caller.
  #
  # Input:
  # sock --> file descriptor of the socket
  # *chunkList --> Buffer to copy data into
  # chunkLength --> Buffer size(set to 1024 bytes for this program specification)
  # remoteHost, remoteLength --> sockaddr parameters
  # numSecs --> Number of secs to wait for a new chunk to arrive(only works if a socket binding is originally made)
  #
  # Output:
  # 0,..,1024 --> Number of bytes read from one chunk
  # -55 --> $ sign detected
  # -98 --> file send error detected
  #----------------------------------------------------------------------
*/

int recvfromWithTimeOut(int sock, char *chunkList, int chunkLength, \
                        struct sockaddr * remoteHost, socklen_t remoteLength, int numSecs)
{
  /* Prepare string to indicate that there is end of input from sender
     Please see README. This is currently disabled because I am assuming
     that the last byte is actually '$' and not "$"(which would be 2 bytes. */
  //char *dollarSign = "$";
  /* Prepare buffer to put 2 bytes of data into in case 2 bytes are read to check for $ */
  char handleDollar[1];
  int bytesRead;
  /* Set up select and ioctl variables */
  int ioctlBytes;
  fd_set sockSet;
  struct timeval timeVal;
  FD_ZERO(&sockSet);
  FD_SET(sock, &sockSet);
  timeVal.tv_sec = numSecs;

  bytesRead = 0;
  ioctlBytes = 0;
  /* Returns > 0 if there is data present within 5 seconds of time */
  if (select(sock + 1, &sockSet, NULL, NULL, &timeVal) > 0)
  {
    /* If there are no bytes read by ioctl, we have an error. */
    if (ioctl(sock,FIONREAD,&ioctlBytes) < 0)
    {
      fprintf(stderr, "Error using ioctl to check number of byte in socket\n");
      return -1;
    }
    else
    {
      /* If exactly two bytes are read, then we need to check if it is a dollar sign */
      if (ioctlBytes == 1)
      {
        bytesRead = recvfrom(sock, handleDollar, 1, 0, (struct sockaddr *) \
                             remoteHost, &remoteLength);
        /* If a dollar sign is read, then return -55 */
        if (handleDollar[0] == '$')
        {
          return -55;
        }
        /* This code is used if you want me to accept a 2 byte string "$".
           if (strcmp(dollarSign, handleDollar) == 0)
           {
           return -55;
           }
        */
        /* Dollar sign not read */
        else
        {
          /* Our chunk consists only of what's inside of handleDollar, so copy the information to chunkList */
          memcpy(chunkList, handleDollar, 1);
          //printf("Chunk list is %s\n", chunkList);
          return 1;
        }
      }
      else
      {
        bytesRead = recvfrom(sock, chunkList, chunkLength, 0, \
                             (struct sockaddr *) remoteHost, &remoteLength);
        return bytesRead;
        /* 5 Bytes are read, so check for file send error which is the fse@ flag sent by the server */
      }

    }
  }
 
  else
  {
    /* Error with select occurred */
    return -1;
  }  
}

int getTimeInSeconds(void)
{
  time_t timeNow;
  struct tm *tm;
  timeNow = time(0);
  if ((tm = localtime (&timeNow)) == NULL)
  {
    return -1;
  }


  return ((tm->tm_hour * 3600) + (tm->tm_min * 60) + (tm -> tm_sec));
}

/*
  # Function convertStringToInt
  #----------------------------------------------------------------------
  # Subroutine convertStringToInt
  # Input:
  # stringCon (a string to be converted to an int)
  #
  # Output:
  # Integer value of a string
  #----------------------------------------------------------------------
*/

int convertStringToInt(char *stringCon)
{
  char *temp;
  long valueToTestNow;

  valueToTestNow = strtol(stringCon, &temp, 10);

  if(errno != 0)
  {
    fprintf(stderr, "String Conversion Error, %s\n", strerror(errno));
    exit(1);

  }
  /* Entire string converted successfully */
  else if (*temp == '\0')
  {
    return (int) valueToTestNow;
  }
  else
  {
    fprintf(stderr, "String did not get completely converted\n");
    usage();
    exit(1);
  }
}


/*
  # Function main
  #----------------------------------------------------------------------
  # Subroutine main:
  # 1 : Takes into parameters ip port filename
  # Main's first job is to potentially attempt to bind to the socket of the server.
  # If binding is successful, then it sends a small string to the server indicating
  # the filename of the file that it is requesting from the server.
  # At this point, one of two things can happen :
  #
  # 1 : If the server cannot find or access the file, it sends back "nf@", and the
  # client will exit at this point as a result indicating <File Not Found>
  # 
  # 2 : If the server finds the file, it will send back "ff@" and the client will wait.
  # Assuming the file was found, the client will wait for the server to tell it the
  # number of bytes of the file. Once it receives the number of bytes of the file,
  # it will allocate this number of bytes and then wait for chunks to start being sent
  #
  # Once chunks are being sent, a byte counter is maintained, which indicates
  # the number of bytes currently read by the client from the server.
  # Also, the chunks are added to the buffer, which is later to be written to the hard drive.
  # 
  # However, the client will exit and post an error if the chunks being 
  # sent ingreater than 5 seconds intervals without
  # finishing the end of the file. However, the client will write the number of 
  # bytes that were read before the 5 second interval of receiving files was 
  # interrupted. It will not attempt to write these bytes to file.
  #
  # When '$' is read, or when a chunk with less than 1024 bytes has been read,
  # then the client knows that it has reached end of file, so it writes these
  # bytes from the buffer to a file on the hard drive.
  # It is important to note that the files are written with a random filename and
  # are stored in /tmp/
  #
  # Input:
  # argv[1] : Ip Number to request file from
  # argv[2] : Port Number to request file from
  # argv[3] : File Name to request file of 
  # Output:
  # 0 : Execution successful
  #----------------------------------------------------------------------
*/
int main(int argc, char *argv[])
{
  /* Initialize sockaddr variables */
  struct sockaddr_in master, from;
  int sock;
  socklen_t fromLen = sizeof(from);
  char *serverIP, *ep, *fileName;
  /* Initialize buf which temporarily stores chunks to be written to the file */
  char buf[BUFFERLENGTH];
  /* receiveFile is where the entire file gets stored */
  char *receiveFile;

  int  bytesRead, totalBytesRead, canWriteToFile, chunksRead;
  u_short port;
  u_long p;

  /* Create a buffer that stores a temporary file name in case the file is written.
     Curerntly a disabled feature since we are not actually writing a file to disk.*/
  char buffer2 [L_tmpnam];
  FILE *fp2;

  if (argc != 4)
  {
    fprintf(stderr, "Invalid number of program arguments\n");
    usage();
  }

  p = strtoul(argv[2], &ep, 10);
  if (*argv[2] == '\0' || *ep != '\0')
  {
    /* The port number either was not a number or it was found to be empty. */
    fprintf(stderr, "%s - not a number\n", argv[1]);
    usage();
  }

  if ((errno == ERANGE && p == ULONG_MAX) || (p > USHRT_MAX))
  {
    /* Port number cannot be stored -- it may be too long.*/
    fprintf(stderr, "%s - value out of range\n", argv[1]);
    usage();
  }

  /* Store the server IP, port number, and file name */
  serverIP = argv[1];
  port = p;
  fileName = argv[3];
  
  /* create a socket */
  sock = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock < 0)
  {
    perror("Server cannot open the master socket.\nClosing now.");
    exit(1);
  }

  /* bind the socket to local addresses of client and let OS pick any port number */

  memset((char *)&master, 0, sizeof(master));
  master.sin_family = AF_INET;
  master.sin_addr.s_addr = htonl(INADDR_ANY);
  master.sin_port = htons(0);

  if (bind(sock, (struct sockaddr *)&master, sizeof(master)) < 0) {
    perror("binding socket failed");
    return 0;
  }       

  /* from is the address of the server that we want to send messages to and
     receive messages from.
     The IP number is converted to binary format using inet_aton */

  memset((char *) &from, 0, sizeof(from));
  from.sin_family = AF_INET;
  from.sin_port = htons(port);
  if (inet_aton(serverIP, &from.sin_addr)==0)
  {
    fprintf(stderr, "Invalid IP Address.\nExiting now\n");
    usage();
  }
  
  printf("Sending file request to %s port %d\n", serverIP, port);

  /* Store the file name into a buffer to be sent to file per request */
  strcpy(buf,fileName);

  if (sendto(sock, buf, strlen(buf), 0, (struct sockaddr *)&from, fromLen)==-1)
    perror("sendto");
  //exit(0);
  
  /* This is necessary for very fast networks(i.e. ethernet at home) because otherwise
   * recvFromWithTimeOut will bug out and not cancel after 5 secs */
  //sleep(0.1);

  printf("Finished sending request\n");
  /* Receive the first response to the server which will be file found or file not found.
     After 10 seconds of sending its initial request, the client will give up and exit if no response
     is given */

  //printf("Waiting to receive bytes from the server\n");

  receiveFile = (char *) malloc(1024);
  /* Create a pointer that points specifically to the start of the file to be written */
  //char *originalPointer = &receiveFile[0];
       
  bytesRead = 0;
  totalBytesRead = 0;
  canWriteToFile = 0;

  /* Get a temporary file name and store this name into buffer2.
     The file will be stored in /tmp/randomname.
     This feature is currently disabled since the client is not receiving the file. */

  
  tmpnam (buffer2);
  fp2 = fopen(buffer2, "a" );
  

  //sleep(0.1);
  chunksRead =0;
  while (!canWriteToFile)
  {
    /* Start receiving the file in chunks -- store each chunk in receiveFile */
    bytesRead = recvfromWithTimeOut(sock, receiveFile, 1024, \
                                    (struct sockaddr *) &from, sizeof(from),5);
    if (bytesRead > 0)
    {
      chunksRead++;
      /* Not an efficient solution, but I am forced to append chunks to end of file by program specifications unfortunately */
      /* Incremement the total bytes read into a file buffer */
      totalBytesRead += bytesRead;
      /* Incrememnt the memory address of receiveFile to be prepard for the next chunk */
      if (bytesRead < 1024)
      {
        /* Read a chunk less than 1024 bytes, so the entire file must have been read */
        canWriteToFile = 1;
      }
    }
    /* Note that recvFromWithTimeOut sends back -55 if dollar sign is seen */
    else if (bytesRead == -55)
    {
      /* Dollar Sign seen, so end of file has been reached
         We will not write $ to the file */
      canWriteToFile = 1;
    }
      
    else
    {
      fprintf(stderr, "Problem Detected.\n");
      fprintf(stderr, "No successful chunk transmissions in the last 5 seconds.\n");
      fprintf(stderr, "Before encountering a problem, we had received ");
      fprintf(stderr, "%d bytes successfully.\n", totalBytesRead);
      fprintf(stderr, "Possible error reasons include :\n");
      fprintf(stderr, "File not foud, file transmission error, etc\n");
      fprintf(stderr, "Exiting now\n");
      fclose(fp2);
      close(sock);
      exit(1);
    }

  }
  fclose(fp2);
  close(sock);
  /* Write the chunks to a file on the hard disk */
  if (bytesRead != -55)
  {
    printf("Received %d bytes successfully from %d chunks.\n", totalBytesRead,\
           chunksRead);
    printf("The last chunk size was %d bytes.\nNo program was written.\n",\
           bytesRead);
  }
  else if (bytesRead == -55)
  {

    printf("Received %d bytes successfully from %d chunks.\n", totalBytesRead,\
           chunksRead);
    printf("The last chunk size was 1 byte('$').\nNo program was written.\n"); 
  }   
  return 0;
}
