// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"

#if defined(CHANGED)
 /* put your changed code here */
#include "synch.h"
#endif

// testnum is set in main.cc
int testnum = 1;

//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------
#if defined(CHANGED) && defined(HW1_NOSEMAPHORES)
/* Exercise 1 changed code before using semaphore */

int SharedVariable; 
void SimpleThread(int which) { 
  int num, val; 
  for(num = 0; num < 5; num++) { 
    val = SharedVariable; 
    printf("*** thread %d sees value %d\n", which, val); 
    currentThread->Yield(); 
    SharedVariable = val+1; 
    currentThread->Yield(); 
  } 
  val = SharedVariable; 
  printf("Thread %d sees final value %d\n", which, val); 
}
 
#elif defined(CHANGED) && defined(HW1_SEMAPHORES) 
/* Exercise 1 changed code using semaphore */
int SharedVariable;

Semaphore sp("HW1_SEMAPHORES", 1);

/* Barrier implementation */
int count = 0;
Semaphore mx("BARRIER_COUNTER_MUTEX", 1);
Semaphore br("BARRIER", 0);
/* Barrier implementation */
 
void SimpleThread(int which) 
{ 
  int num, val; 
  for(num = 0; num < 5; num++) {
    // semaphore P signal
    sp.P();

    val = SharedVariable; 
    printf("*** thread %d sees value %d\n", which, val); 
    currentThread->Yield(); 
    SharedVariable = val+1;

    // semaphore V signal
    sp.V();

    currentThread->Yield(); 
  }

  /* barrier */
  mx.P();
  count = count + 1;
  mx.V();

  if(count == testnum)
    br.V();
  br.P();
  br.V();
  /* barrier */

  val = SharedVariable; 
  printf("Thread %d sees final value %d\n", which, val); 
}

#elif defined(CHANGED) && defined(HW1_LOCKS) 
/* Exercise 2 changed code using Lock */
int SharedVariable;

Lock lk("HW1_LOCKS");

/* Barrier implementation */
int count = 0;
Semaphore mx("BARRIER_COUNTER_MUTEX", 1);
Semaphore br("BARRIER", 0);
/* Barrier implementation */
 
void SimpleThread(int which) 
{ 
  int num, val; 
  for(num = 0; num < 5; num++) {
    // semaphore P signal
    lk.Acquire();

    val = SharedVariable; 
    printf("*** thread %d sees value %d\n", which, val); 
    currentThread->Yield(); 
    SharedVariable = val+1;

    // semaphore V signal
    lk.Release();

    currentThread->Yield(); 
  }

  /* barrier */
  mx.P();
  count = count + 1;
  mx.V();

  if(count == testnum)
    br.V();
  br.P();
  br.V();
  /* barrier */

  val = SharedVariable; 
  printf("Thread %d sees final value %d\n", which, val); 
}

#else
/* the original code goes here */
void
SimpleThread(int which)
{
    int num;
    
    for (num = 0; num < 5; num++) {
	printf("*** thread %d looped %d times\n", which, num);
        currentThread->Yield();
    }
}
#endif

//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");

    Thread *t = new Thread("forked thread");

    t->Fork(SimpleThread, 1);
    SimpleThread(0);
}


#if defined(CHANGED)

//----------------------------------------------------------------------
// ThreadTest (n)
// 	Invoke a test routine. Fork n *new* threads, which means there 
//      will be n + 1 threads running
//----------------------------------------------------------------------

void ThreadTest(int n) {
  testnum = n + 1;
  for (int i = 1; i <= n; ++i) {
    DEBUG('t', "Entering ThreadTest %d\n", i);
    Thread *t = new Thread("forked thread");

    t->Fork(SimpleThread, i);
  }
  SimpleThread(0);
}
#else
/* the original code goes here */
void
ThreadTest()
{
    switch (testnum) {
    case 1:
	ThreadTest1();
	break;
    default:
	printf("No test specified.\n");
	break;
    }
}
#endif
