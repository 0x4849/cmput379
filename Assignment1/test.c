/* **************************************************
# Author: Brad Harrison         			        *
# Lecture: B1         								*
# Class : CMPUT 379                                 *
# Lab TA: Mohomed Shazan Mohomed Jabbar             *
#                                                   *
# Lecturer: Dr. Mohammad Bhuiyan                    *
# Created: February 1st, 2015                       *
#***************************************************/

#include "memchunk.h"
#define MEMCHUNKNUMBER 30
#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

/*
# Function main
#----------------------------------------------------------------------
# Subroutine main performs the following functions:
# 1 : Allocates memory for an array of memchunk structs called chunk_list
# 2 : Calls get_mem_layout to write to this array of structs
# 3 : Runs through each value for each struct in the array and prints them out.
#
# In my opinion, the code is less readable due to an 80 character per line
# limit. It is unnatural to read printf statements on two separate lines.
#
# Input:
# argc and argv, which are not used.
# Output:
# 0 to indicate everything went okay.
#----------------------------------------------------------------------
*/
int main (int argc, char *argv[])
{ 
  int memchunkListIndex = 0;
  int numberOfMemChunks;
  int indextoCheck;
  struct memchunk * chunk_list;
  
  printf("\n");
  printf("Allocating %i chunks for chunk_list as defined by MEMCHUNKNUMBER\n",\
  MEMCHUNKNUMBER); 
 
  chunk_list = (struct memchunk *) \
  malloc(sizeof(struct memchunk) * MEMCHUNKNUMBER);
  if (chunk_list == NULL) 
  {
    fprintf(stderr, "Couldn't allocate enough memory for malloc.\n");
  }
  
  /* Determine the number of chunks in memory after running get_mem_layout */
  numberOfMemChunks = get_mem_layout(chunk_list, MEMCHUNKNUMBER);
  printf("The total number of memory chunks found by get_mem_layout was %i\n", \
  numberOfMemChunks);
  
  indextoCheck = min(numberOfMemChunks,MEMCHUNKNUMBER);
  printf("Printing stored chunk info for %i total chunks\n",indextoCheck);
  
  /* Print the new memchunk struct information for each struct inside of 
   * the array called chunk_list */
  while(memchunkListIndex < indextoCheck)
  {
    printf("\n");
    printf("Printing chunk information for memory chunk #%i\n",\
    memchunkListIndex+1);
    
    /* First chunk in memory always starts at 0x00000000 */
    if (memchunkListIndex == 0)
    {
      printf("The starting address for the chunk is: 0x00000000\n");
    }
    else
    {
    printf("The starting address for the chunk is: %p\n", \
    (void *)chunk_list[memchunkListIndex].start);
    }
    
    printf("The length of the chunk and its number of bytes is: %lu\n",\
     chunk_list[memchunkListIndex].length);
    
    printf("The number of pages inside of the chunk is %lu\n", \
    chunk_list[memchunkListIndex].length/getpagesize());
    
    printf("The memory access level for the corresponding chunk is: %d\n", \
    chunk_list[memchunkListIndex].RW);
    
    /* Determine the accessibility of the memchunk struct and 
     * print the corresponding information */
    switch (chunk_list[memchunkListIndex].RW) 
    {
    
    case 0 :
      printf("The chunk and its memory addresses are read-only\n");
      break;
    
    case 1 :
      printf("The chunk and its memory addresses are read-writable\n");
      break;
    
    case -1 :
      printf("The chunk and its memory addresses are inaccessible\n");
      break;
    
    default:
      fprintf(stderr, "The chunk has an invalid accessibility level.\n");
      break;
    }
      
    memchunkListIndex++;
  }
  
  return 0;
}
