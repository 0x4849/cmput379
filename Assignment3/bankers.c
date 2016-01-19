#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <signal.h>

/* **************************************************
   # Author: Brad Harrison                             *
   # Lecture: B1         				               *
   # Class : CMPUT 379                                 *
   # Lab TA: Mohomed Shazan Mohomed Jabbar             *
   #                                                   *
   # Lecturer: Dr. Mohammad Bhuiyan                    *
   # Created: April,7 2015                             *
   #***************************************************/
   
void signalHandler(int signum);

void signalHandler(int signum)
{
  
  fprintf(stderr,"\n\nCtrl C detected. Ending simulation now.\n\n");
  exit(0);
}

/*Generate random C code taken from http://faq.cprogramming.com/cgi-bin/smartfaq.cgi?answer=1042005782&id=1043284385*/
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

/* This function is called to check if all of the processes have been allocated after using
   the Banker's algorithm. It is only called when no changes have been made
   for an entire iteration of the Banker's algorithm. */

int allFinished(int numProc, int *isFinishedProc)
{
  int i;
  for (i = 0; i < numProc; i++)
  {
    if (!isFinishedProc[i])
    {
      return 0;
    }
    
  }
  return 1;
}

/* This function is called during the Banker's algorithm. Essentially, it works
   as follows : during the banker's algorithm, we need to increment the
   work or available resources array after we find out it is safe to allocate
   a particular resources */
void incrementWork(int *tmpWork, int *tmpAllocation, int offSet, int numRes)
{
  int i;
  for (i = 0; i < numRes; i++)
  {
    /* Simply increment the work array by whatever was storred in the allocation array
       before we found out that it was safe to allocate a resource. This is because
       we need to release the resources back to work after allocating a resource to
       a process during the Banker's Algorithm */
    tmpWork[i] = tmpWork[i] + tmpAllocation[offSet+i];
  }
}

/* This function is also called during the Banker's algorithm to see if it is
   safe to allocate a particular resource to a process. The way it checks this
   is just by making sure that the remaining resources required for a process is not
   greater than the currently available resources. The banker's algorithm
   is only interested in allocating resources to processes that are able
   to finish with the given resources. It is not interested in allocating resources
   otherwise. */

int safeToAllocate(int *tmpWork, int *tmpNeed, int numRes, int offSet)
{
  int i;

  for (i=0; i < numRes; i++)
  {
    if (tmpWork[i] - tmpNeed[offSet+i] <0)
    {
      return 0;
    }

  }
  return 1;
}

/* This is the entire Banker's Algorithm, which essentially works as follows :
   It attempts to find a process whereby Need_i <= Work and the process is not
   finished yet inside of the algorithm. If it is not able to find such a
   process, it checks if all processes are finished -- if they are, then
   the system is in a safe state and we are able to grant the request.
   If not, the syste mis not in a safe state, and we cannot grant the request. */

int performBankersAlgorithm(int *tmpNeed, int *tmpWork, int *tmpAllocation, int *isFinishedProc, int numProc, int numRes)
{
  int i;
  int isNotSafe;
  int didNothing;
  
  i = 0;
  isNotSafe = 1;

  /* While we are not yet in a safe state, try to find a process whereby
     Need_i <= Work */
  while(isNotSafe)
  {
    i = 0;
    /* didNothing is a simple flag that lets us know if the process has allocated
       any resources during the current loop iteration */
    didNothing = 0;
    while (i < numProc)
    {
      /* try to find a process whereby
         Need_i <= Work and the current process is not finished yet*/
      if (!isFinishedProc[i] && safeToAllocate(tmpWork, tmpNeed, numRes, i*numRes))
      {
        /* We found such a process, so set the process to finished and
           release its allocated resources back to work */
        isFinishedProc[i] = 1;
        incrementWork(tmpWork, tmpAllocation, i*numRes, numRes);
        /* We did something so set the flag to 1 */
        didNothing = 1;
      }
      i++;
    }
    if (!didNothing)
    {
      /* Since we did nothing in this loop iteration, check if all processes
         are finished, and if so, declare safe state */
      if (allFinished(numProc, isFinishedProc))
      {
        /* Set isNotSafe to false since we returned a safe state ! */
        isNotSafe = 0;
      }
      break;
    }
  }

  if (isNotSafe)
  {
    /* We are not in a safe state, so return 0. */
    return 0;
  }
  else
  {
    /* We are in a safe state, so return 1. Passed Banker's Algorithm */
    return 1;
  }
  

}

/* This program is essentially used to perform changes as necessary when allocating
   a resource to a process. For example, we have allocation = allocationI + requestedI
   needI = maxI - AllocationI, and finally :
   workI = workI - requestedI.

   However, this program is called both when an allocation is granted
   (Then the arrays passed in are not temporary arrays but real permanent arrays)
   And also when a request for allocation is made. The reason is that in order
   to perform a request for allocation, the system must pretend that the request
   has actually been made and then see if work becomes negative for example --
   in which case, it would be an impossible fulfillment. */
   
int makeInitialChangesToWorkAndNeed(int numRes, int offSet, int *tmpAllocation, int *tmpRequested, int *tmpMax, int *tmpNeed, int *tmpWork)
{
  int i;
  i = 0;

  while (i < numRes)
  {

    tmpAllocation[offSet] = tmpAllocation[offSet] + tmpRequested[offSet];
    tmpNeed[offSet] = tmpMax[offSet] - tmpAllocation[offSet];

      
    tmpWork[i] = tmpWork[i] - tmpRequested[offSet];

    i ++;
    offSet ++;
      

    /* Work is negative so we have to return 0 to indicate that the request for allocation
       was not successful. This is only a simple test that lets us bypass the Banker's
       Algorithm if it is obvious that we won't have enough resources to fulfill the
       original request */
    if (tmpWork[i-1] < 0)
    {
      return 0;
    }
        
  }

  return 1;
}

/* This function can be viewed as a proxy between the full Banker's algorithm
   and the calling subroutine. It first pretends to grant the allocation request
   by calling makeInitialChangesToWorkAndNeed with its temporary arrays, and
   if nothing goes wrong, it performs Banker's Algorithm. If B.A. is successful,
   it returns -- safe for allocation. If B.A. is not successful, it returns
   not safe for allocation */
int checkSafeState(int *workArray, int *needArray, int *maxArray, int *allocationArray, int *requestedResources, int *waitingQueue, int numProc, int numRes, int offSet)
{
  int *tmpWork, *tmpNeed, *tmpMax, *tmpAllocation, *tmpRequested, *tmpFinished;

  int totalBytesToAllocate;

  totalBytesToAllocate = numProc*numRes*sizeof(int);
  
  tmpAllocation = malloc(totalBytesToAllocate);
  tmpWork = malloc(numRes * sizeof(int));
  tmpMax = malloc(totalBytesToAllocate);
  tmpNeed = malloc(totalBytesToAllocate);
  tmpFinished = malloc(numProc * sizeof(int));
  tmpRequested = malloc(totalBytesToAllocate);


  memcpy(tmpAllocation,allocationArray, totalBytesToAllocate );

  memcpy(tmpWork,workArray, numRes * sizeof(int) );
  memcpy(tmpNeed,needArray, numProc*numRes * sizeof(int) );
  memcpy(tmpMax,maxArray, numProc*numRes * sizeof(int) );

  memcpy(tmpRequested,requestedResources, numProc*numRes * sizeof(int) );
  memset((int*)tmpFinished, 0, numProc * sizeof(int));

  if (!makeInitialChangesToWorkAndNeed(numRes, offSet, tmpAllocation, tmpRequested, \
                                       tmpMax, tmpNeed, tmpWork))
  {
    /* The initial request can be denied here because it forces the work array into
       negative values */
    return 0;
  }

  
  if (performBankersAlgorithm(tmpNeed, tmpWork, tmpAllocation, tmpFinished, numProc, numRes))
  {
    /* B.A. was successful, so return 1 for safe to allocate */
    return 1;
  }
  else
  {
    return 0;
  }
    
}

/* This is simply a helper function for printCurrentSnapshot to help it print
   all of the values it needs to print from arrayToPrint */
void printNumericalValues(int numRes, int offSet, int *arrayToPrint)
{
  int j;
  j = 0;
  
  while (j < numRes)
  {
    printf("%d ", arrayToPrint[offSet]);
    offSet++;
    j++;

  }  
}

/* This is the primary function associated with printing a snapshot.
   It should be noted that the snapshot will only look OKAY if the terminal
   is in full-screen mode, and it was only tested on 1600 x 1200, so I do not
   know forsure if it will look okay or not on a lower resolution */

void printCurrentSnapshot(int *allocationArray, int *requestedResources, int *workArray, int *maxArray, int *needArray, int *waitingArray, int *maxAvailableResources, int numProc, int numRes)
{
  int i, j, offSet;
  printf("Current snapshot:\n");
  printf("\n");
  printf("\t\t\t\t\t\tCURRENTLY\t\tMAXIMUM\t\tMAXIMUM\n");
  printf("\t\tCURRENT\t\tCURRENT\t\tAVAILABLE\t\tPOSSIBLE\tAVAILABLE\n");
  printf("\t\tALLOCATION\tREQUEST\t\tRESOURCES\t\tREQUEST\t\tRESOURCES\n");
  printf("\t\tR0..R%d\t\tR0..R%d\t\tR0..R%d\t\t\tR0..R%d\t\tR0..R%d\n",numRes, numRes, numRes, numRes, numRes);

  i = 0;
  while (i < numProc)
  {
    offSet = i * numRes;
    j = 0;
    printf("P%d\t\t",i+1);
    
    printNumericalValues(numRes, offSet, allocationArray);
    printf("\t\t");

  
    printNumericalValues(numRes, offSet, requestedResources);
 
    printf("\t\t");

    /* If we are the top row of the snapshot, then print the work array */
    if (i == 0)
    {
      printNumericalValues(numRes, offSet, workArray);
    }
    printf("\t\t\t");

    /* The maximum possible request corresponds to the maxArray values as opposed
       to the need array values, which might be more intuitive. */
    printNumericalValues(numRes, offSet, maxArray);
    printf("\t\t");

    /* If we are at the top row of the snapshot, then print the max available resources */
    if (i == 0)
    {
      printNumericalValues(numRes, offSet, maxAvailableResources);
    }
    
    printf("\n");
    i++;
  }

  printf("\n");
  
}

/* This function is simply called one time only -- at the start of the simulation
   to calculat ethe maximium available resources.
   It can work with any resources allocated in the beginning */
void computeMaxAvailableResources(int *maxAvailableResources, int *allocationArray, int *workArray, int numProc, int numRes)
{
  int i;
  int j;
  int k;
  memset((int*)maxAvailableResources, 0, numRes * sizeof(int));
 
  for (i = 0; i < numProc; i++)
  {
    k = i;
    j = 0;
    while (j < numRes)
    {
      maxAvailableResources[i] = maxAvailableResources[i] + allocationArray[k];

      j ++;
      k = k + numRes;
    }
    maxAvailableResources[i] = maxAvailableResources[i] + workArray[i];
  }

}

/* This function will handle print to terminal requests for :
   Current Request for Allocation Granted and Current Request not Granted
*/
void printRequest(int numRes, int *requestedResources, int offSet, char *stringToPrint)
{
  int i;
  printf("Request (");
  for (i = 0; i < numRes; i++)
  {
    printf("%d", requestedResources[offSet+i]);
    if (i < numRes-1)
      printf(", ");

  }
  printf(") ");
  printf("%s\n", stringToPrint);

}

/* This helper function will handle print requests granted or not granted from processes
   that are in the waiting queue after we attempt to re-allocate the resources when
   we release a particular process's resources */

void printRequestQueue(int numRes, int *requestedResources, int offSet, char *stringToPrint)
{
  int i;
  printf("Previous request of  (");
  for (i = 0; i < numRes; i++)
  {
    printf("%d", requestedResources[offSet+i]);
    if (i < numRes-1)
      printf(", ");

  }
  printf(") ");
  printf("%s\n", stringToPrint);

}

/* After a request has been made and granted, we need to remove the
   resources that correspond to that particular request in the request
   array and set it to 0 */

void removeRequestedResources(int numRes, int *requestedResources, int offSet)
{
  int i;

  for (i = 0; i < numRes; i++)
  {
    requestedResources[offSet+i] = 0;

  }

}

/* This helper function ensures that two edge cases get dealt with :
   If a process wants to release all 0 resources, then we simply
   deny this request and it ask it to pass its turn in the loop.
   Similarly, if a process wants to request all 0 resources, then
   we need to deny this request as well.

   Otherwise, we would end up in a situation where we requested 0 resources
   and somehow ended up in a waiting queue because we couldn't satisfy
   the requests of others through the Banker's Algorithm */

int allResourcesAreZero(int numRes, int offSet, int *requestedResources)
{
  int i;

  for (i = 0; i < numRes; i++)
  {
    /* Note that requestedResources can either be the allocation array
       (In case we want to release resources), or it can be the
       requestedResources array (in case we want to request resources) */
    if (requestedResources[offSet+i])
    {
      return 0;
    }
  }
  return 1;
}

/* This helper function handles the actual request for releasing resources,
   which requires the allocation array to be decremented by the number
   of resources that were requested for releasal, and it requires
   the workArray to be incremented by this same amount */
void releaseTheResources(int *releaseResourcesArray, int *allocationArray, int *workArray, int offSet, int numRes)
{
  int i;

  for (i = 0; i < numRes; i++)
  {
    allocationArray[offSet+i] = allocationArray[offSet+i] - releaseResourcesArray[i];
    workArray[i] = workArray[i] + releaseResourcesArray[i];
  }
  
}

/* This helper function is simply used to print to the terminal when
   resources have been released */
void printReleaseResources(int *releaseResources, int numRes)
{
  int i;
  printf("(");
  for (i = 0; i < numRes; i++)
  {
    printf("%d", releaseResources[i]);
    if (i < numRes-1)
    {
      printf(",");
    }
      
  }
  printf(") resources\n");

}

/* This function is called when we have released some resources, and we
   want to know if we can satisfy all requests currently in the waiting queue.
   Even if we can only satisfy the first one, this is fine. */

void tryToSatisfyAllRequests(int numRes, int numProc, int *waitingQueue, int *requestedResources, int *allocationArray, int *workArray, int *needArray, int *maxArray, int *maxAvailableResources)
{
  int isSafeToAllocateRes;
  int offSet;
  int i;
  char reqGranted[256];
  char reqNotGranted[256];
  
  for (i = 0; i < numProc; i++)
  {
    offSet = i * numRes;
    if (waitingQueue[i])
    {
      /* Check if we can reach a safe state for satisfying the request
         on the waiting queue */
      isSafeToAllocateRes = checkSafeState(workArray, needArray, maxArray, allocationArray, requestedResources, waitingQueue, numProc, numRes, offSet);

      if (isSafeToAllocateRes)
      {
        /* We reached a safe state, so go ahead and satisfy the request on the queue */
        waitingQueue[i] = 0;
        sprintf(reqGranted, "from P%d has been satisfied", i+1);
        printRequestQueue(numRes, requestedResources, offSet, reqGranted); 
        makeInitialChangesToWorkAndNeed(numRes, offSet, allocationArray, requestedResources, maxArray, needArray, workArray);
        /* Remember to clear out requestedResources since request has been granted. */
        removeRequestedResources(numRes,requestedResources, offSet);      
      }
      else
      {
        sprintf(reqNotGranted, "from P%d cannot be satisfied, P%d is still in waiting state", i+1, i+1);
        printRequestQueue(numRes, requestedResources, offSet, reqNotGranted);
        /* Add the process back to the waiting queue since we could not satisfy the
           request */
        waitingQueue[i] = 1;          
      }
    }
  }
  /* Print a snapshot after we have released resources -- regardless of if we
     satisfied any requests on the waiting queue or not */
  printCurrentSnapshot(allocationArray, requestedResources, workArray, maxArray, needArray,waitingQueue, maxAvailableResources, numProc, numRes);

}

/* This function was never really asked for, but I included it anyways :
   If no request can be satisfied because all processes are in the waiting queue,
   then the system will print to the terminal -- warning the user that there was
   a deadlock, and it will exit the program. */
void checkIfDeadLock(int *waitingQueue, int numProc)
{
  int i;
  for (i = 0; i < numProc; i++)
  {
    if (!waitingQueue[i])
      return;

  }
  printf("We have a deadlock situation because every process is in a waiting state.\n");
  printf("With the way the Banker's Algorithm works,\n");
  printf("If all processes cannot be satisfied for NEED requirement -- no allocation takes place\n");
  printf("Here we cannot satisfy all processes, so we have an indefinite deadlock\n");
  printf("Exiting now\n");
  exit(0);
}

/* This is the function that is called every 5 seconds, and essentially it needs
   to request a random action, then try to perform the random action for each
   process that is not currently in a waiting state */

void runProcesses(int *allocationArray, int *workArray, int *maxArray, int *requestedResources, int *needArray, int *waitingQueue, int *maxAvailableResources, int *releaseResources,  int numProc, int numRes)
{

  int isAllZero;
  int offSet;
  int i, j, k, isSafeToAllocateRes;
  int requestedRandomAction;
  int requestRandomRes;
  int *tempWork;

  char reqGranted[100], reqNotGranted[100], curRequest[100];


  offSet = 0;
  checkIfDeadLock(waitingQueue, numProc);
  for (i=0; i < numProc; i++)
  {
    /* Let 0 = Do Nothing, 1 = Request Resources, 2 = Release Resources */
    /* Return random value [0,3) */
    
    if (waitingQueue[i] == 1)
    {
      /* If the process is currently in the waiting queue, then skip execution */
      continue;
    }

    requestedRandomAction = getRandomValue(0, 3);
    /* If we want to just continue execution, then go ahead and do so */
    if (requestedRandomAction == 0)
    {
      continue;
    }

    /* This will correspond to a resource release request */
    else if (requestedRandomAction == 2)
    {
      /* If all of our allocated resources are currently zero, and the
         process still wanted to release some resources, then we simply
         continue execution, and we skip this part.

         It is not really necessary -- because we could simply
         release 0 resources each time and then try to satisfy remaining
         requests on the waiting queue.

         So this here is open for interpretation. Would a process
         with zero resources allocated ever tell the operating system
         that it wishes to resource its resources? In my opinion, no,
         so that it is why I have chosen to skip execution here. */
        
      if (allResourcesAreZero(numRes, i * numRes, allocationArray))
      {
        continue;
      }
      else
      {
        offSet = i * numRes;
        j = 0;
        isAllZero = 1;
          
        while (j < numRes)
        {
          requestRandomRes = getRandomValue(0, allocationArray[offSet]+1);
          if (requestRandomRes)
          {
            isAllZero = 0;
          }
          releaseResources[j] = requestRandomRes;
          offSet++;
          j++;
        }
        /* Another situation completely open for interpretation.
           Here we are releasing 0 resources of each resource type.
           The professor told me that it is not valid to request 0
           resources of each type, so in my opinion, it is not valid to
           release all 0 resources of each type, so if the request involves
           all zero resources, then simply skip execution. */
        if (isAllZero)
        {
          continue;
        }

        printf("P%d has released ", i+1);
        printReleaseResources(releaseResources, numRes);
        releaseTheResources(releaseResources, allocationArray, workArray, i*numRes, numRes);

        /* After resources have been released, it is necessary to update
           the allocation, work, need arrays */
        makeInitialChangesToWorkAndNeed(numRes, i*numRes, allocationArray, requestedResources, maxArray, needArray, workArray);

        /* Try to satisfy all requests on the waiting queue since we have released
           resources */
        tryToSatisfyAllRequests(numRes, numProc, waitingQueue, requestedResources, allocationArray, workArray, needArray, maxArray, maxAvailableResources); 
      }

    }

    /* The requested action was to request resources, so this will start a chain of
       events that includes using the Banker's algorithm if needed to see if the
       system can remain in a safe state or not */
    else if (requestedRandomAction == 1)
    {

      offSet = i * numRes;
      j = 0;
      while (j < numRes)
      {
        requestRandomRes = getRandomValue(0, needArray[offSet]+1);
        requestedResources[offSet] = requestRandomRes;
        offSet++;
        j++;
      }


      /* All of the resources requested of each instance was 0.
         The professor told me that this request is not valid, so I am
         skippping execution here */
      
      if (allResourcesAreZero(numRes, i*numRes, requestedResources))
      {
        continue;
      }

      /* Print a snapshot of the system after the request has been made */
      printCurrentSnapshot(allocationArray, requestedResources, workArray, maxArray, needArray,waitingQueue, maxAvailableResources, numProc, numRes);

      sprintf(curRequest, "came from P%d", i+1);
      printRequest(numRes, requestedResources, i*numRes, curRequest); 
      printf("\n");
      offSet = i * numRes;
      
      /* Check to see if the system is in a safe state for giving the request or not */
      isSafeToAllocateRes = checkSafeState(workArray, needArray, maxArray, allocationArray, requestedResources, waitingQueue, numProc, numRes, offSet);

      if (isSafeToAllocateRes)
      {
        /* if it's safe to allocate the resource, then print snapshot and make the
           changes necessary to the given arrays */
        sprintf(reqGranted, "from P%d has been granted", i+1);
        printRequest(numRes, requestedResources, offSet, reqGranted); 
        makeInitialChangesToWorkAndNeed(numRes, offSet, allocationArray, requestedResources, maxArray, needArray, workArray);
        /* Remember to clear out requestedResources since request has been granted. */
        removeRequestedResources(numRes,requestedResources, offSet);

        /* print snapshot after request was OK'd */
        printCurrentSnapshot(allocationArray, requestedResources, workArray, maxArray, needArray,waitingQueue, maxAvailableResources, numProc, numRes);
          
      }
      else
      {
        /* Request not satisfied -- add the process to the waiting queue */
        sprintf(reqNotGranted, "from P%d cannot be satisfied, P%d is in waiting state", i+1, i+1);
        printRequest(numRes, requestedResources, offSet, reqNotGranted);
        waitingQueue[i] = 1;          
      }
       

    }

     
        
  }
}

/* Substring code from http://www.linuxquestions.org/questions/programming-9/extract-substring-from-string-in-c-432620/ */
/* This is a helper function that is used to parse the input at the beginning */
char* substring(const char* str, size_t begin, size_t len)
{
  
  
  if (str == 0 || strlen(str) == 0 || strlen(str) < begin )
  {
    printf("Returning 0 -- invalid parameter given\n");
    return 0;
  }
  
  return strndup(str + begin, len);
}


/* This function simply updates the maxArray after the input in the beginning.
   The work array calls it as well */

void storeTheData(char detailsOfPx[256], int *maxArray, int onResIndex)
{
  int theDigit;
  int i;
  int startTime;
  int oldStart;
  int length;


  i = 0;
  oldStart = 0;
  length = strlen(detailsOfPx);

  while (i < length)
  {

    if (isspace(detailsOfPx[i]))
    {

      char *subs = substring(detailsOfPx, oldStart, i);
      theDigit = atoi(subs);

      maxArray[onResIndex] = theDigit;
      onResIndex++;
      oldStart = i + 1;
    }
      
    i++;
  }


  char *subs2 = strndup(detailsOfPx+oldStart, i+1);
  theDigit = atoi(subs2);
  maxArray[onResIndex] = theDigit;
  
}


int main(char **argv, char argc)
{
  int *allocationArray, *workArray, *maxArray, *requestedResources, *needArray, *waitingQueue, *maxAvailableResources, *releaseResources;
  int numProc;
  int numRes;
  int onProc;
  int i;
  int startTime;
  int simTime;
  int totalBytesToAllocate;
  int numSecs;
  
  char resourceTypes[256];
  char numInstances[256];
  char numProcesses[256];
  char detailsOfPx[256];

  /* Set up the signal handler to handle CTRL C requests */
  (void) signal(SIGINT, signalHandler);

  /* Clear out the arrays before using them */
  memset((char*)&resourceTypes, 0, sizeof(resourceTypes));
  memset((char*)&numInstances, 0, sizeof(numInstances));
  memset((char*)&numProcesses, 0, sizeof(numProcesses));
  memset((char*)&detailsOfPx, 0, sizeof(detailsOfPx));

  /* Gather input from user -- assuming correct input only */
  printf("Number of different resource types: ");
  scanf(" %[^\n]", resourceTypes);
  numRes = atoi(resourceTypes);
  numProc = atoi(resourceTypes);
  
  printf("Number of instances of each resource type :");
  scanf(" %[^\n]", numInstances);

  
  printf("Number of processes: ");
  scanf(" %[^\n]", numProcesses);
  

  numProc = atoi(numProcesses);

  
  
  totalBytesToAllocate = numProc*numRes*sizeof(int);

  /* Allocate memory for a number of different arrays that are used in this program */
  allocationArray = malloc(totalBytesToAllocate);

  workArray = malloc(numRes * sizeof(int));
  maxArray = malloc(totalBytesToAllocate);
  needArray = malloc(totalBytesToAllocate);
  waitingQueue = malloc(numProc * sizeof(int));
  requestedResources = malloc(totalBytesToAllocate);
  maxAvailableResources = malloc(numRes * sizeof(int));
  releaseResources = malloc(numRes * sizeof(int));

  /* Clear out all of the useful arrays before using them */
  memset((int*)allocationArray, 0,totalBytesToAllocate);
  memset((int*)workArray, 0, numRes * sizeof(int));
  memset((int*)releaseResources, 0, numRes * sizeof(int));
  memset((int*)maxAvailableResources, 0, numRes * sizeof(int));
  memset((int*)maxArray, 0, totalBytesToAllocate);
  memset((int*)waitingQueue, 0, numProc * sizeof(int));
  memset((int*)requestedResources, 0, totalBytesToAllocate);
  
  /* Store the data from the max array */
  storeTheData(numInstances, workArray, 0);
  for (i = 0; i < numProc; i++)
  {
    onProc = i * numRes;
    
    printf("Details of P%d: ",i+1);
    scanf(" %[^\n]", detailsOfPx);

    storeTheData(detailsOfPx, maxArray,  onProc);
  }
  computeMaxAvailableResources(maxAvailableResources, allocationArray, workArray, numProc, numRes);

  /* The need array is simply a copy of the max array in the beginning since
     we started with zero resources allocated */
  memcpy(needArray,maxArray, numProc*numRes * sizeof(int) );
  startTime = getTimeInSeconds();
  simTime = startTime;
  
  numSecs = 4;
  printf("Hello. Please read the README if you can. Thank you. :) \n");
  while (1)
  {
    /* Every five seconds, we make an action for each process -- but it is 4 the first time */
    if (getTimeInSeconds() - simTime >= numSecs)
    {
      simTime = getTimeInSeconds();
      /* Run all of the processes in a loop whereby each chooses one possible
         action */
      runProcesses(allocationArray, workArray, maxArray,\
                   requestedResources, needArray, waitingQueue, maxAvailableResources, releaseResources, numProc, numRes);
    }
    numSecs = 5;

  }

  return 0;

}

 
      

