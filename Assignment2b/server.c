/* *****************************************************
   # Author: Brad Harrison              			   *
   # Lecture: B1        							   *
   # Class : CMPUT 379                                 *
   # Lab TA: Mohomed Shazan Mohomed Jabbar             *
   #                                                   *
   # Lecturer: Dr. Mohammad Bhuiyan                    *
   # Created: March 18th, 2015                         *
   #***************************************************/

/*
 * Copyright (c) 2008 Bob Beck <beck@obtuse.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* server.c  - the "classic" example of a socket server */

/*
 * compile with gcc -o server server.c
 * or if you are on a crappy version of linux without strlcpy
 * thanks to the bozos who do glibc, do
 * gcc -c strlcpy.c
 * gcc -o server server.c strlcpy.o
 *
 */

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

#define ROUTERBUFFERSIZE 3000
#define CHUNKSIZE 1024

struct sockaddr_in from;
char *clientIP;
int clientPort;

static void usage()
{
  extern char * __progname;
  fprintf(stderr, "usage: %s portnumber documentsdir logfile\n", __progname);
  exit(1);
}

static void kidhandler(int signum)
{
  /* signal handler for SIGCHLD */
  waitpid(WAIT_ANY, NULL, WNOHANG);
}

/*Code snippet adapted from http://www.tutorialspoint.com/c_standard_library/c_function_strftime.htm*/
void getCurrentTime(char firstTime[80])
{
  time_t rawTime;
  struct tm * timeInformation;
  time (&rawTime);
  timeInformation = localtime ( &rawTime );
  /*Store raw time as HH:MM:SS AM or PM*/
  strftime(firstTime, 80, "%I:%M:%S%p", timeInformation);
}

int sendAllChunks(int sock, struct sockaddr * destinationValue, \
	socklen_t destinationLength, int bytesToSend, FILE *fileToSend)
{  
  /* Set up a buffer to read 1024 bytes from file into and then send it to client */  
  char *senderBuffer = (char *) malloc(bytesToSend);
   
  int currentBytesRead;
  int newBytesRead, newBytesSent, finalByteSent;
  /* Use this if you want to send a 2 byte string called "$" */
  char dollarBuffer[] = "$";
    
  currentBytesRead = 0;
  while(currentBytesRead < bytesToSend)
  {
    //printf("sending new bytes %d\n", currentBytesRead);
    /* Read bytes into senderBuffer with the size of CHUNKSIZE(1024 bytes in this case) */
    newBytesRead = fread(currentBytesRead + senderBuffer,\
	 1, CHUNKSIZE, fileToSend);
  
    /* If we only read 0 bytes from the file, then some error would have occurred. This is because we only enter the loop
       if the number of bytes read is less than the file size */
    if (newBytesRead == 0)
    {
      free(senderBuffer);
      return -1;
    }
      
    newBytesSent = sendto(sock, currentBytesRead + senderBuffer, newBytesRead,\
	 0, (struct sockaddr *) destinationValue, destinationLength);

    /* If we sent 0 bytes, then an error has occurred. Return -2 to log <file transmission error>*/
    if (newBytesSent == 0)
    {
      free(senderBuffer);
      perror("sendto");
      return -2;
    }
      
    currentBytesRead += newBytesRead;
    sleep(0.1);
      
  }
  /* By this point in time, we have already sent all of the file to the client.
     If the number of bytes of the file is equivalent to 0 in mod 1024, then we need to send '$' */
  if (bytesToSend % 1024 == 0)
  {
    finalByteSent = sendto(sock, dollarBuffer, 1, 0, \
	(struct sockaddr *) destinationValue, destinationLength);
    /* An error sending the $. We should log this as <file transmission error> as well */
    if (finalByteSent == 0)
    {
      free(senderBuffer);
      perror("sendto");
      return -3;
    }
  }
  free(senderBuffer);
  return bytesToSend; 
}

/* Given a small string, send it to the client */
int sendTheFile (int sock, char numBytesString[15], int lengthToSend,\
	 int flags, struct sockaddr * destinationValue, socklen_t length)
{
  return sendto(sock, numBytesString, lengthToSend, flags,\
	 (struct sockaddr *) destinationValue, length);
  
}

/* Update the log by taking in the file name, log file file, time the request was sent and some string finalParam
   finalParam can vary completely :
   If the file transmission was successful, then it will be the end time of transmitting a file.
   If the file transmission was not successful, then <file not found> or <file transmission error> will be inside *finalParam */

void updateLog(char *originalFinalName, FILE *logFileToWrite, \
	char *logFile, char firstTime[80], char *finalParam)
{

  int fd;
  char buf[80];
  int stringSize;

  /* Concatenate all of the required strings for logging into buf */
  sprintf(buf,"%s %d %s %s %s\n", clientIP, clientPort,\
          originalFinalName, firstTime, finalParam);
  stringSize = strlen(buf)+1;
  /* Form a new string using malloc and copy buf over to it. */
  char *lineToLog = (char *) malloc(stringSize);
  strcpy(lineToLog, buf);

  /* Open the file for append mode if it exists */
  fd = open(logFile, O_CREAT|O_APPEND|O_WRONLY, 07777);
  if (fd == -1)
  {
    fprintf(stderr, "Internal Server Error : Cannot Read From Logfile.\n");
    return;
  }

  /* Create a spinlock that provides an exclusive lock via flock.
     I think that flock internally creates a while(true) loop, so
     this is probably unnecessary to create one here, but it doesn't
     matter. */
  
  while(1)
  {
    if(flock(fd, LOCK_EX) == 0)
    {
      /* Do not include the 0 byte when writing to the log file. */
      write(fd, lineToLog, stringSize-1);
      close(fd);
      flock(fd, LOCK_UN);
      break;
    }
  }
  free(lineToLog);
 
  
}

/* handleFile is what sends the chunks to the client and updates the log.
   It does the majority of the bulk of work of the server */

void handleFile(int sock, struct sockaddr * destinationValue,\
  socklen_t destinationLength, char *fullFilePath, char *originalFileName, \
  char *logFile, char firstTime[80], char lastTime[80])
{
  /* Initialize a stat struct that is used primarily to calculate the number of byte */
  struct stat fileBytes;
  /* numBytes contains the number of bytes of the file */
  int numBytes;
  FILE *fileToSend, *logFileToWrite;
  char finalParam[80];

  /* Check the file to send for read permissions so that we can determine if we need to log file not found */
  fileToSend = fopen(fullFilePath, "r");
  if (fileToSend == NULL)
  {
    /* File not found */
    if (errno == ENOENT)
    {
      strcpy(finalParam,"<file not found>");
    }
    /* File was found but we dont have access to it */
    else if (errno == EACCES)
    {
      strcpy(finalParam,"<file not found>");
    }
    /* Some error reading the file occurred */
    else
    {
      strcpy(finalParam,"<file not found>");
    }

    /* Assuming any of these errors occured, update the corresponding log file */
    updateLog(originalFileName, logFileToWrite, logFile, \
	firstTime, finalParam);
  }
  else
  {
    if (stat(fullFilePath, &fileBytes) == -1)
    {
      strcpy(finalParam, "<file not found>");
      updateLog(originalFileName, logFileToWrite, logFile, \
	firstTime, finalParam);
      perror("stat");
    }
    else
    {
      /* Get the number of bytes of the file, which the server uses to know
         when to stop sending the file. */
      numBytes = (long) fileBytes.st_size;

      /* Try to send all the chunks of the file now */
      int numBytesRead =  sendAllChunks(sock, destinationValue, \
	destinationLength, numBytes, fileToSend);

      if (numBytesRead < 0)
      {
        /* If the sendAllChunks function returns an error(<0), then something went wrong at some point in time,
           so log file transmission error. This also encompasses sending 0 bytes as an error, which returns -1. */
           
        strcpy(finalParam, "<file transmission error>");
        updateLog(originalFileName, logFileToWrite, logFile, firstTime, finalParam);
        
        if (numBytesRead == -1)
        {
          fprintf(stderr, "Internal Server Error : Cannot Read From File\n");
          return;
        }
        else if (numBytesRead == -2)
        {
          fprintf(stderr, "Failed to Send a Data Chunk(s) to Client.\n");
          return;
      
        }
        else if (numBytesRead == -3)
        {
          fprintf(stderr, "Failed to Send a Dollar Symbol to Client.\n");
          return;
        }
        return;
      }
      else
      {
        /* Assuming that none of these errors occur, then it's time to get the
           current time at the end of transmission and then to log everything[
           as successful */
        getCurrentTime(lastTime);
        updateLog(originalFileName, logFileToWrite, logFile,\
	 firstTime, lastTime);
        return;
      }
    }
  }
}

/* handleClientRequest is called by main and then it does a bit of work and sets up handleFile request */
void handleClientRequest(int sock, struct sockaddr *destinationValue, \
  socklen_t destinationLength, char *directoryPath, \
  char *logFile,  char *childBuffer)
{

  char lastTimeBuffer[80];
  char *fullFilePath;
  char firstTime[80];
  

  /* Get the current time and store it into firstTime buffer.
     This will be needed later on when logging file transfers */
  getCurrentTime(firstTime);

  /* Concatenate the full file path into one string */
  fullFilePath = (char *) malloc(strlen(directoryPath)\
	+strlen(childBuffer) + 1);
  strcpy(fullFilePath, directoryPath);
  strcat(fullFilePath, childBuffer);

  /* Call handleFile to finish handling the file request */
  handleFile(sock, destinationValue, destinationLength, \
	fullFilePath, childBuffer, logFile, firstTime, lastTimeBuffer);
}

int main(int argc,  char *argv[])
{
  /* Initialize some initial values
     including socket values */
  
  socklen_t fromLength;
  struct stat directoryPath;
  struct  sockaddr_in master, from;
  char *ep;
  struct sigaction sa;

  u_short port;
  pid_t pid;
  u_long p;
  int sock, receiveBytes;
  char routerBuffer[ROUTERBUFFERSIZE];
  char *childBuffer;

  char *directoryPathway, *logFile;


  if (argc != 4)
    usage();
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
    fprintf(stderr, "%s - value out of range\n", argv[1]);
    usage();
  }

  port = p;
  directoryPathway = argv[2];
  logFile = argv[3];

  /*See if the supplied directory path exists as a directory to get files from*/
  /*Code based upon http://cboard.cprogramming.com/c-programming/132228-checking-if-char-*path%3B-valid-directory.html*/
  if (stat(directoryPathway, &directoryPath) < 0)
  {
    fprintf(stderr, "Supplied dir arg : %s didn't exist.\n", directoryPathway);
    exit(1);
  }

  /* More checking for a valid directory. The previous was more concerned if the supplied filename(directory) existed.
     The current is more focused on whether it can be accessed as a directory */
  else if (!S_ISDIR(directoryPath.st_mode))
  {
    fprintf(stderr, "Supplied dir arg : %s was not a directory.\n", \
	directoryPathway);
    exit(1);
  }

  /* Set up the socket variables */
  fromLength = sizeof(from);
  memset((char*)&master, 0, sizeof(master));
  sock = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock < 0) {
    perror ("Server: cannot open master socket");
    exit (1);
  }

  master.sin_family = AF_INET;
  master.sin_addr.s_addr = htonl(INADDR_ANY);
  master.sin_port = htons (port);

  if (bind (sock, (struct sockaddr*) &master, sizeof (master)))
  {
    perror ("Server: cannot bind master socket");
    exit (1);
  }

  /*
   * first, let's make sure we can have children without leaving
   * zombies around when they die - we can do this by catching
   * SIGCHLD.
   */
  sa.sa_handler = kidhandler;
  sigemptyset(&sa.sa_mask);
  
  /*
   * we want to allow system calls like accept to be restarted if they
   * get interrupted by a SIGCHLD
   */
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1)
    err(1, "sigaction failed");


  printf("Setting server up and listening for connections on port %u\n", port);
   
  if (daemon(1, 1) == -1)
  {
    err(1, "daemon() failed");
  }

  /* Main Loop for server -- receive requests here and deal with them through children */
  for(;;)
  {

    /* Receive a string from the client requesting a specific file name to be sent */
    receiveBytes = recvfrom(sock, routerBuffer, ROUTERBUFFERSIZE, 0, \
                            (struct sockaddr *)&from, &fromLength);
    //printf("router received %d bytes from the UDP\n", receiveBytes);
    if (receiveBytes == -1)
    {
      err(1, "Failed to read bytes from the receiver.");
    }
    
    routerBuffer[receiveBytes] = 0;
    //printf("received the following message: \"%s\"\n", routerBuffer);
    
    /*
     * We fork child to deal with each connection, this way more
     * than one client can connect to us and get served at any one
     * time.
     */

    pid = fork();
    if (pid == -1)
      err(1, "fork failed");

    if(pid == 0)
    {
      clientIP =  inet_ntoa(from.sin_addr);
      clientPort = ntohs(from.sin_port);
  
      //fprintf(stderr, "fork:[%d]\nClient IP %s:%d\n", getpid(), clientIP, clientPort);

      childBuffer = (char *) malloc(strlen(routerBuffer)+1);
      /* Copy the buffer information from the client into a child buffer.
         Technically, each child has its own "routerBuffer" variable,
         so this is only done to ensure that the strings are properly
         formatted */
      strcpy(childBuffer, routerBuffer);

      /* Handle the client request -- give the file name(childBuffer) and everything else needed
         to ensure that the file is properly sent */
      handleClientRequest(sock, (struct sockaddr *) &from, sizeof(from),\
	 directoryPathway, logFile, childBuffer);

      /* The child should exit now that it is done */
      exit(0);
      
    }

  }
  close(sock);
  return 0;
}
 
