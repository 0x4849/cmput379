/* **************************************************
# Author: Brad Harrison         			        *
# Lecture: B1        								*
# Class : CMPUT 379                                 *
# Lab TA: Mohomed Shazan Mohomed Jabbar             *
#                                                   *
# Lecturer: Dr. Mohammad Bhuiyan                    *
# Created: February 1st, 2015                       *
#***************************************************/

/*===========================================================================*
 * memchunk.h                   
 *        
 * Provided memchunk struct that I have been asked to place inside of memchunk.h 
 * for CMPUT 379 Assignment 1
 *  
 *          
 *===========================================================================*/

#ifndef MEMCHUNK_H_
#define MEMCHUNK_H_

/* Library Dependencies */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <setjmp.h>
#include <sys/user.h>

/* Global type definition */
typedef unsigned char uint1; 

/* memchunk struct used to store memory chunk attributes */
struct memchunk 
{
  void *start;
  unsigned long length;
  int RW;
};

/* memchunk.c function prototypes*/
int get_mem_layout (struct memchunk *, int);
int getReadability(uint1 *);
int readPrivs(uint1 *);
int writePrivs(uint1 *);
void signalHandler(int);

#endif /* MEMCHUNK_H_ */
