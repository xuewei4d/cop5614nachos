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

#elif defined(CHANGED) && defined(HW1_ELEVATOR)

unsigned int max_floors = 0;
unsigned int current_floor = 1;
const unsigned int capacity = 5;
unsigned int room = 5; // available space in the elevator
enum DIRECTION {UP, DOWN, IDLE, CLOSE};
DIRECTION direction = UP;

unsigned int *waiting_list; // for each floor the number of passengers waiting outside the elevator
unsigned int *going_list; // for each floor, the number of passenger going inside the elevator
Lock elevator_lk("ELEVATOR_LOCK");
Lock token_lk("TOKEN_LOCK");
Condition passenger_out_cd("PASSENGER_OUT_CONDITION"); // elevator notify passengers to go out of the elevator
Condition passenger_in_cd("PASSENGER_IN_CONDITION");  // elevator notify passengers to get into the elevator
Condition elevator_active_cd("ELEVATOR_ACTIVE_CONDITION"); // elevator notify passengers it is active

// check braodcasting of condtion variable, if condition variable is not Hoar-Style.
// make Mesa condition variable acts like Hoare-Style condtion variable
Condition finish_broadcast_cd("FINISH_BROADCAST_CONDTION");
unsigned int broadcast_count = 0;

unsigned int token = 0; // generate passengers' id
struct Passenger {
  unsigned int at_floor, to_floor, id;
  DIRECTION direction;
};

//----------------------------------------------------------------------
// map_floor
//
//    maps the real floor to the index of waiting_list and going_list
//
//----------------------------------------------------------------------
unsigned int map_floor(DIRECTION d, unsigned int f) {
  if (d==UP || f == 1)
    return f;
  else
    return 2*max_floors - f;
}

//----------------------------------------------------------------------
// searchNextFloor
//
//    finds the next floor the elevator needs to stop, if 
// 1. there is a passenger waiting on that floor
// 2. there is a passenger in the elevator going to that floor.
//
//----------------------------------------------------------------------
unsigned int searchNextFloor(DIRECTION d, unsigned int f, unsigned room) {
  unsigned int i, j;
  i = map_floor(d, f);
  j = i ;
  do {
    if ((room != 0 && waiting_list[j] != 0) || going_list[j] != 0)
      break;
    j = (j+1) %(2*max_floors-1);
  } while (j != i);

  return j;
}

//----------------------------------------------------------------------
// realNextFloor
//
//    maps the index of the waiting_list and going_list to the real floor
//
//----------------------------------------------------------------------

inline unsigned int realNextFloor(unsigned int next_floor) {
  if (next_floor > max_floors - 1)
    return 2*max_floors - next_floor;
  else
    return next_floor;
}


void ElevatorThread(int num_floors) {

  // notify passengers the elevator is ready
  elevator_lk.Acquire();
  elevator_active_cd.Signal(&elevator_lk);
  direction = UP;
  printf("Elevator ready.\n");
  elevator_lk.Release();

  currentThread->Yield();
  while (true) {

    // compute the next_floor the elevator should go.
    elevator_lk.Acquire();
    unsigned int next_floor = searchNextFloor(direction, current_floor, room);
    unsigned int real_next_floor  = realNextFloor(next_floor);
    int num_waiting = waiting_list[next_floor];
    int num_going = going_list[next_floor];
#ifdef HW1_ELEVATOR_DEBUG
    printf("DEBUG elevator current floor %d, next_floor %d, direction %d.\n", current_floor, next_floor, direction);
    printf("DEBUG waiting %d, going %d, room %d.\n", num_waiting, num_going, capacity - room);
    for (int z = 0; z < 2*max_floors - 1; ++z)
      printf("%d ", waiting_list[z]);
    printf("\n");
    for (int z = 0; z < 2*max_floors - 1; ++z)
      printf("%d ", going_list[z]);
    printf("\n");
#endif
    if (real_next_floor == current_floor && num_waiting == 0 && num_going == 0) {
      // if nobody waiting neither outside of the elevator nor in the elevator 
      // close the elevator and quit
      direction = CLOSE;
      printf("Elevator stoped services.\n");
      delete [] waiting_list;
      delete [] going_list;
      elevator_lk.Release();
      break;
    }
    else if (real_next_floor == current_floor && (num_waiting != 0 || num_going != 0)) {
      // elevator opens the door and begin the service
      printf("Elevator arrives on floor %d.\n", current_floor);

      printf("Elevator let passengers out.\n");

      // broadcast all waiting threads, 
      // and must waiting for the responds from all waiting threads
      broadcast_count = capacity - room;
      passenger_out_cd.Broadcast(&elevator_lk);
      if (broadcast_count != 0) {
	finish_broadcast_cd.Wait(&elevator_lk);
#ifdef HW1_ELEVATOR_DEBUG
	printf("finish broadcast\n");
#endif
      }
      
      printf("Elevator let passengers in.\n");
      broadcast_count = 0;
      for(int z = 0; z < 2*max_floors - 1; ++ z)
	broadcast_count += waiting_list[z];
      passenger_in_cd.Broadcast(&elevator_lk);
      if (broadcast_count != 0) {
	finish_broadcast_cd.Wait(&elevator_lk);
#ifdef HW1_ELEVATOR_DEBUG
	printf("finish broadcast\n");
#endif
      }

      printf("Elevator closed the door at floor %d.\n", current_floor);
      elevator_lk.Release();
    }
    else {
      // next_floor != current_floor, the elevator moves up or down

      for (int i = 0; i < 50; ++i);
      if (real_next_floor > current_floor) {
	direction = UP;
	++ current_floor;
      }
      else {
	direction = DOWN;
	-- current_floor;
      }

      elevator_lk.Release();
    }
    
  }
}

//----------------------------------------------------------------------
// Elevator
//    initializes the parameters of the elevator and 
//    starts a the running thread of the elevator
//
//----------------------------------------------------------------------
void Elevator(int numFloors) {
  max_floors = numFloors;
  current_floor = 1;
  direction = IDLE;
  waiting_list = new unsigned int [2*max_floors - 1];
  going_list = new unsigned int [2*max_floors - 1];
  for (unsigned int i = 0; i < 2*max_floors - 1; ++i) {
    waiting_list[i] = 0;
    going_list[i] = 0;
  }
  Thread *elevator = new Thread("Elevator Thread");

  elevator->Fork(ElevatorThread, numFloors);
  
}

//----------------------------------------------------------------------
// PassengerThread
//
//    the running thread of the passenger
//
//----------------------------------------------------------------------

void PassengerThread(int pPassenger) {

  Passenger *p = (Passenger*)pPassenger;

  // waiting for the elevator is ready
  // if the elevator is closed, then quit
  elevator_lk.Acquire();
  if (direction == IDLE)
    elevator_active_cd.Wait(&elevator_lk);
  else if (direction == CLOSE) {
    printf("Passenger %d finds out the elevator is closed.\n", p->id);
    elevator_lk.Release();
    return;
  }

  //  register the at_floor and to_floor into the waiting _list
  waiting_list[map_floor(p->direction, p->at_floor)] += 1;
  printf("Passenger %d wants to from %d go to %d floor.\n", p->id, p->at_floor, p->to_floor);

  // whatever current_floor equlas at_floor or not, the passengers should wait for signal of "passenger_in" first
  // if awaken, the passenger check the current_floor then available space in the elevator.
  while (true) {
    passenger_in_cd.Wait(&elevator_lk);       // waiting for elevator signal let passenger get in
    broadcast_count -= 1;
    if (broadcast_count == 0)
      finish_broadcast_cd.Signal(&elevator_lk);

    if (current_floor == p->at_floor) {
      // if there is available room, update the waiting_list and going_list
      if (room > 0) {
	  room --;
	  waiting_list[map_floor(p->direction, p->at_floor)] -= 1;
	  going_list[map_floor(p->direction, p->to_floor)] += 1;
	  break;
      } 
      
    }
  }

  printf("Passenger %d (from %d, to %d) get into floor %d.\n", p->id, p->at_floor, p->to_floor, current_floor);
    
  // passenger waiting in the elevator.
  // if awaken, check the current_floor against its to_floor
  // if they are equal, get out of the elevator.
  while (true) {
    passenger_out_cd.Wait(&elevator_lk);
    broadcast_count -= 1;
      
    if (broadcast_count == 0) 
      finish_broadcast_cd.Signal(&elevator_lk);

    if (current_floor == p->to_floor)  {
      room ++;
      going_list[map_floor(p->direction, p->to_floor)] -= 1;
      
      printf("Passenger %d (from %d, to %d) get out of floor %d. \n", p->id, p->at_floor, p->to_floor, current_floor);
      break;
    }
  }

  elevator_lk.Release();
  delete p;   // destroy the structure that p points
}

//----------------------------------------------------------------------
// ArrivingGoingFromTo
//
//    initialize the passenger information, and starts a running thread of
// the passenger
//
//----------------------------------------------------------------------

void ArrivingGoingFromTo(int atFloor, int toFloor) {
  if (atFloor < 1 || atFloor > max_floors || toFloor < 1 || toFloor > max_floors || atFloor == toFloor) {
    printf("Exception: passenger illegal atFloor %d, toFloor %d.\n", atFloor, toFloor);
    return;
  }
    
  Thread *thread = new Thread("Passenger Thread");
  Passenger *p = new Passenger;
  p->at_floor = atFloor;
  p->to_floor = toFloor;
  token_lk.Acquire();
  p->id = ++token;
  token_lk.Release();
  if (atFloor < toFloor)
    p->direction = UP;
  else if (atFloor > toFloor)
    p->direction = DOWN;
  thread->Fork(PassengerThread, (int)p);
}

#elif defined(CHANGED)
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

#ifndef HW1_ELEVATOR
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
#endif
