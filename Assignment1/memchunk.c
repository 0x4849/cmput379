/* **************************************************
# Author: Brad Harrison           			        *
# Lecture: B1        								*
# Class : CMPUT 379                                 *
# Lab TA: Mohomed Shazan Mohomed Jabbar             *
#                                                   *
# Lecturer: Dr. Mohammad Bhuiyan                    *
# Created: February 1st, 2015                       *
#***************************************************/

#include "memchunk.h"
/* Global variable for sigjmp buffer */
static sigjmp_buf env;  

/*
# Function readPrivs
#----------------------------------------------------------------------
# Subroutine readPrivs takes in an address and determines whether or not
# it has read privileges by trying to read its value into charTester.
# 
# In order to check for memory access, the subroutine overrides default
# segmentation fault signal behavior as indicated by declaring a 
# signal handler called signalHandler and passing in the SIGSEGV parameter
# to handle possible segmentation fault errors.
#
# To check for Read Access, the subroutine initializes an unsigned character 
# called charTester, which tries to assign its value to the dereferenced
# location of address. If the memory is not readable, then signalHandler
# will replace signalVariable by 1, so the else portion of the conditional
# gets executed, and -1 is returned to identify an inccessible part of 
# logical memory.
# If the memory is readable, then 1 is returned to indicate this. 
#
# Input : 
# Pointer to unsigned character in memory called address
# Output :
# 1 :Read-only access for the logical memory address
#-1: Inaccessible access for the logical memory address
#----------------------------------------------------------------------
*/

int readPrivs(uint1 * address)
{
  uint1 charTester;
  int signalVariable;
  
  /* Override default signal behavior for segmentation faults corresponding
   * to SIGSEGV signals to behavior assigned by subroutine signalHandler */
  (void) signal(SIGSEGV, signalHandler);
  
  signalVariable = sigsetjmp(env, 1);
  if (signalVariable == 0)
  {
    /* Test address for read access by assigning charTester to address */
    charTester = *address;
    /* Address was readable, so return 1 to indicate read-access memory */
    return 1;
  }
  /* Address is not readable, so return -1 to indicate inaccessible memory */
  else
  {
    return -1;
  }
}

/*
# Function writePrivs
#----------------------------------------------------------------------
# Subroutine writePrivs takes in an address and determines whether or not
# it has write privileges by re-writing its value to its old value.
# 
# In order to check for memory access, the subroutine overrides default
# segmentation fault signal behavior as indicated by declaring a 
# signal handler called signalHandler and passing in the SIGSEGV parameter
# to handle possible segmentation fault errors.
#
# If the address in logical memory has read access, then the subroutine 
# tests for write access by assigning the location in memory to its old
# value which is indicated by charTester. 
# If the subroutine does not have write access, then
# signalHandler replaces signalVariable by 1, and the else portion of
# the conditional gets executed, and the subroutine returns 0 to 
# indicate that the address in logical memory has read-only access.
# 
# Input : 
# Pointer to unsigned character in memory called address
# Output :
# 0 : Read-only access for the logical memory address
# 1 : Read-write access for the logical memory address
#----------------------------------------------------------------------
*/

int writePrivs(uint1 * address)
{
  uint1 charTester;
  int signalVariable;
  
  /* Backup the old value of address- works fine since it has read privileges */
  charTester = *address;
  
  /* Override default signal behavior for segmentation faults corresponding
   * to SIGSEGV signals to behavior assigned by subroutine signalHandler */
  (void) signal(SIGSEGV, signalHandler);
  
  signalVariable = sigsetjmp(env, 1);
  /* Test address for write access by assigning adress back to charTester */
  if (signalVariable == 0)
  {
    *address = charTester;
    /* Address was both readable and writable, return 1 to indicate RW memory */
    return 1;
  }
  /* Address is not writable, so return 0 to indicate read-only memory */
  else
  {
    return 0;
  }
}

/*
# Function getReadability 
#----------------------------------------------------------------------
# Subroutine getReadability takes in a pointer to an unsigned character
# called address, and it tests which permissions this particular address
# has in logical memory. The three possibilities in logical memory are
# Read Access, Read/Write Access, and Inaccessible.
#
# To check for Read Access, the subroutine calls another subroutine called
# readPrivs, which will return 1 for read access and -1 for inaccessible memory.
# Assuming readPrivs returns -1, then getReadability will also return -1
# to indicate an inaccessible memory address.
#
# If the address in logical memory has read access(readPrivs reuturned 1), 
# then the subroutine tests for write access by calling writePrivs, which
# will return 0 for read-only memory and 1 for read-write memory.
# getReadAbility will return this value to indicate how accessible the memory is.
# 
# Input : 
* Pointer to unsigned character in memory called address
# Output :
# -1 : Inaccessible logical memory address
#  0 : Read-only access for the logical memory address
#  1 : Read-write access for the logical memory address
#----------------------------------------------------------------------
*/

int getReadability(uint1 * address) 
{
  int isReadable;
  /* Override default signal behavior for segmentation faults corresponding
   * to SIGSEGV signals to behavior assigned by subroutine signalHandler */
  (void) signal(SIGSEGV, signalHandler);
  
  isReadable = readPrivs(address);
  if (isReadable == -1)
  {
    return -1;
  }
  return writePrivs(address);
}

/*
# Function get_mem_layout
#----------------------------------------------------------------------
# Subroutine get_mem_layout takes in a pointer to a struct memchunk called 
# chunk_list, which has a group of size memchunk structs, which are contigous 
# in memory. Anotherwords, chunk_list is an array of structs whose number of
# struct array elements is defined by the integer parameter variable size.
# 
# The goal of this subroutine is to test for the accessibility levels of 
# all 32 bit addresses in logical memory. That is, to test for accessibility
# of addresses 0x0000 0000 to 0xFFFF FFFF.
#
# When a group of contigous addresses in memory share the same accessibility
# level, they are called a "chunk" of memory, and chunk_list's job(barring
# the number of current chunks not being greater than size) is to
# store all starting addresses of each chunk to a void pointer "start"
# inside of a memchunk struct. It also assigns the range of the number of
# bytes of memory for each corresponding chunk and assigns this number of
# bytes to variable "length" inside of memchunk struct.
# 
# Finally, it assigns the accessibility level of each chunk to the variable
# RW inside of the corresponding memchunk struct. RW is assigned to the following
# accessibility level values:
# -1 : Indicating an inaccessible chunk of memory
#  0 : Indicating a read-only chunk of memory
#  1 : Indicating a read-write chunk of memory.
#  
# Input : 
# Pointer to the first memchunk struct in memory as indicated by *chunk_list
# Intger variable size : Indicates the amount of possible chunks
# which have been allocated to mem_chunk structs.
# 
# Output :
# integer chunkTotal : The number of chunks found in logical memory.
#----------------------------------------------------------------------
*/

int get_mem_layout (struct memchunk *chunk_list, int size)
{ 
  /* Get the page size of the system since each page in logical memory
   * has the same accessibility level for each byte in each corresponding
   * page in logical memory.
   */
  int pageSize = getpagesize();
  /* address gets assigned to a pointer to unsigned character starting at 0 */
  uint1 *address = (uint1*) 0x0000000;
  uint1 *prevAddress = (uint1*) 0x00000000;
  uint1 *finalMaxAddress = (uint1 *) 0xFFFFFFFF;
   
  int currentAccessibilityFlag;
  int oldAccessibilityFlag = 99;
  int chunkTotal = 0;
  
  struct memchunk *pointerArithmetic;
  
  while (address < finalMaxAddress && address >= prevAddress)
  {
    
    /* Get the accessibility level for the current address by calling
     * the subroutine getReadability. */ 
    currentAccessibilityFlag = getReadability(address);

    /* Assign the address of pointer to memchunk called pointerArithmetic  
     * to the starting address of chunk_list + the current number of chunks.
     * This address is the current memchunk struct that is being assigned
     * values to. */    
    pointerArithmetic = chunk_list + chunkTotal;
    
    /* If the next address has the same accessibility as the previous address,
     * then they are part of the same chunk in memory */
    if (oldAccessibilityFlag == currentAccessibilityFlag)
    {
  
      /* If we are currently looking at a memchunk that is within the confines
       * of the memory allocated to chunk_list, then increase the corresponding
       * memchunk struct's byte length by a full page size in logical memory. */
      if (chunkTotal <= size)
      {
        /* This is the same thing as writing chunk_list[chunkTotal-1].length */
        (*(pointerArithmetic-1)).length += pageSize;
      }
    }
    /* If the next address does not have the same accessibility as the previous
     * address, then they are not part of the same chunk in memory, so we need
     * to allocate a new chunk to chunk_list if we have the size to do so. */ 
    else
    {
      /* We have encountered a new chunk, so increment the total regardless of
       * having enough size to store the details of this chunk in chunk_list */
      chunkTotal += 1;
      
      if (chunkTotal <= size)
      {
        /* Assign the starting length of the chunk to the size of a page
         * in logical memory since each byte in a page shares the same
         * accessibility level as each other. */ 
        (*pointerArithmetic).length = pageSize;
        /* Assign the starting address of the chunk to start */
        (*pointerArithmetic).start = (uint1 *) address;
        /* Assign the accessibility level to RW */
        (*pointerArithmetic).RW = currentAccessibilityFlag;
      }
    }
    
    oldAccessibilityFlag = currentAccessibilityFlag;
    prevAddress = address; 
    /* Increment the current address by page size since each address in
     * the current page size has already had its accessibility level tested
     * Because each address shares the same accessibility as the starting 
     * address in a page of logical memory. */
    address += pageSize;
  }
  
  return chunkTotal;
}

/*
# Function signalHandler
#----------------------------------------------------------------------
# Subroutine signalHandler takes in a value signo, which corresponds to the
# signal number for SIGSEGV as this subroutine overrides the default behavior
# of segmantation faults, which correspond to SIGSEGV signals in the given
# program. 
# signalHandler simply jumps back to the point in memory as defined by
# sigsetjmp, and it replaces the sigsetjmp value by 1, and this allows
# enough control flow to handle signals inside getReadability(), which is 
# the corresponding calling function. 
#
# Input:
# int signo : Corresponds to SIGSEGV's signal integer as defined in signal.h
# Output:
# None
#----------------------------------------------------------------------
*/

void signalHandler(int signo)
{
  siglongjmp(env, 1);
}


