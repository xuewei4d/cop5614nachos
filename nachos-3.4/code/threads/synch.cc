// synch.cc 
//	Routines for synchronizing threads.  Three kinds of
//	synchronization routines are defined here: semaphores, locks 
//   	and condition variables (the implementation of the last two
//	are left to the reader).
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "system.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	Initialize a semaphore, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//	"initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(char* debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    queue = new List;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	De-allocate semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	Wait until semaphore value > 0, then decrement.  Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------

void
Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
    
    while (value == 0) { 			// semaphore not available
	queue->Append((void *)currentThread);	// so go to sleep
	currentThread->Sleep();
    } 
    value--; 					// semaphore available, 
						// consume its value
    
    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}

//----------------------------------------------------------------------
// Semaphore::V
// 	Increment semaphore value, waking up a waiter if necessary.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that threads
//	are disabled when it is called.
//----------------------------------------------------------------------

void
Semaphore::V()
{
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    thread = (Thread *)queue->Remove();
    if (thread != NULL)	   // make thread ready, consuming the V immediately
	scheduler->ReadyToRun(thread);
    value++;
    (void) interrupt->SetLevel(oldLevel);
}

#if defined(CHANGED) && defined(HW1_LOCKS)

//----------------------------------------------------------------------
// Lock::Lock
// 	Initialize a lock, so that it can be used for synchronization.
//
//      "debugName" is an arbitrary name, usefuly for debugging.
//----------------------------------------------------------------------

Lock::Lock(char* debugName) : name(debugName), isHeld(FALSE), lockHolder( NULL), queue(new List)
{
  
}

//----------------------------------------------------------------------
// Lock::~Lock
// 	De-allocate Lock, when no longer needed.  Assume no one
//	is still waiting on the Lock!
//----------------------------------------------------------------------

Lock::~Lock()
{
  delete queue;
}

//----------------------------------------------------------------------
// Lock::Acquire
// 	Wait until isHeld == FALSE, then set isHeld TRUE, lockHodler as 
//      currentThread.   
//
//----------------------------------------------------------------------

void Lock::Acquire()
{
  IntStatus oldLevel = interrupt->SetLevel(IntOff);
  while (isHeld) {
    queue->Append((void *)currentThread);
    currentThread->Sleep();
  }
  isHeld = TRUE;
  lockHolder = currentThread;
  (void) interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// Lock::Release
// 	 If the lockHoder attemps to release the lock, set isHeld FALSE, 
//       lockHolder NULL, waking up a waiter if necessary; otherwise,
//       the lock cannot be released.
//----------------------------------------------------------------------

void Lock::Release() 
{
  Thread *thread;
  IntStatus oldLevel = interrupt->SetLevel(IntOff);
  if (isHeldByCurrentThread()) {
    DEBUG('t',"The Lock is held by current thread!");

    thread = (Thread *)queue->Remove();
    if (thread != NULL)
      scheduler->ReadyToRun(thread);
    isHeld = FALSE;
    lockHolder = NULL;
  }
  else
    DEBUG('t',"Error: The Lock is held by another thread or not acquired!");
  (void) interrupt->SetLevel(oldLevel);
  
}

//----------------------------------------------------------------------
// Lock::isHeldByCurrentThread
//	Test if the lockHolder is attempting to release the lock
//----------------------------------------------------------------------

bool Lock::isHeldByCurrentThread()
{
  return lockHolder == currentThread;
}
#else
// Dummy functions -- so we can compile our later assignments 
// Note -- without a correct implementation of Condition::Wait(), 
// the test case in the network assignment won't work!

Lock::Lock(char* debugName) {}
Lock::~Lock() {}
void Lock::Acquire() {}
void Lock::Release() {}
#endif

#if defined(CHANGED) && defined(HW1_CONDITIONS)

//----------------------------------------------------------------------
// Condition::Condition
// 	Initialize a condition object.
// 	Assume "no one waiting".
//----------------------------------------------------------------------


Condition::Condition(char* debugName) 
{
	name=debugName;
	queue=new List;
}

//----------------------------------------------------------------------
// Condition::~Condition
// 	Deallocate a conditon object, when it is no longer needed.
//----------------------------------------------------------------------


Condition::~Condition() 
{
	delete queue;
}

//----------------------------------------------------------------------
// Condition::Wait
// 	This function waits for a condition to become free and then 
// 	acquires the conditionLock for the current thread.
//----------------------------------------------------------------------

void Condition::Wait(Lock* conditionLock) 
{ 
	//ASSERT(FALSE);

	IntStatus oldLevel = interrupt->SetLevel(IntOff); //disable interrupt 

	if(conditionLock->isHeldByCurrentThread())        //checking if the lock is hold by the current thread
	{
		conditionLock->Release();  
		DEBUG('t',"***%s about to wait on %s\n", currentThread->getName(), name);
		queue->Append((void *)currentThread);     //After lock was released, appending the current thread to 
							  //condition queue
		currentThread->Sleep();                   //The thread will be blocked
		conditionLock->Acquire();                 //When the thead is awoken, reacquire a lock.
	}
	(void) interrupt->SetLevel(oldLevel);	// re-enable interrupts

}

//----------------------------------------------------------------------
// Condition::Signal
// 	 This function wakes up one of the threads that is waiting on
// 	 the condition.
//----------------------------------------------------------------------

void Condition::Signal(Lock* conditionLock) 
{
	Thread *thread;
	IntStatus oldLevel = interrupt->SetLevel(IntOff);  // disable interrupts
	if(coditionLock->isHeldByCurrentThread())
	{
		if(!queue->IsEmpty())                      // when condition waiting queue is not empty, continue
		{
			thread = (Thread *)queue->Remove();   //wake up a thread waiting for the condition
			if(thread != NULL)
			{
				scheduler->ReadyToRun(thread);//make the thread ready
				DEBUG('t',"***%s signals on %s\n", currentThread->getName(), name);
			}
		}
	}
	(void) interrupt->SetLevel(oldLevel);                 // re-enable interrupts
	       
}

//----------------------------------------------------------------------
// Condition::Broadcast
// 	 This function wakes up all threads that are waiting on
// 	 the condition.
//----------------------------------------------------------------------
void Condition::Broadcast(Lock* conditionLock) 
{
	Thread *thread; 
	IntStatus oldLevel = interrupt->SetLevel(IntOff);     // disable interrupts
	if(conditionLock->isHeldByCurrentThread())
	{
	   	while(!queue->IsEmpty())                      //wake up all threads waiting for the conditon
		{	
			thread = (Thread *)queue->Remove();
			if(thread != NULL)
				scheduler->ReadyToRun(thread); // make thread ready
				DEBUG('t',"***%s broadcasts on %s\n", currentThread->getName(), name);
		}
	}
	(void) interrupt->SetLevel(oldLevel);                 // re-enable interrupts
}

#else
Condition::Condition(char* debugName) { }
Condition::~Condition() { }
void Condition::Wait(Lock* conditionLock) { ASSERT(FALSE); }
void Condition::Signal(Lock* conditionLock) { }
void Condition::Broadcast(Lock* conditionLock) { }
#endif
