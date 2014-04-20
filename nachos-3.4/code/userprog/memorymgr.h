#ifndef MEMORYMGR_H
#define MEMORYMGR_H

#include "bitmap.h"
#include "synch.h"
#include "processmgr.h"

class BitMap;
class Lock;

class MemoryMgr {
 public:
  MemoryMgr();                              // initialization
  ~MemoryMgr();
  int GetPage(int pid, int sharedPage = -1);// return first clear page; if not return -1, and skip the shared page
  void ClearPage(int pageNumber);           // reset page
  unsigned int GetNumFreePages();           // number of free pages
  void UnsetShare(int physicalPage, int PID);
  void SetShare(int physicalPage, int PID);

 private:
  BitMap memoryMap;
  Lock memoryLock;
  int frametime[NumPhysPages];
  bool framePID[NumPhysPages][PIDMAP_SIZE]; // bruteforce for insufficient data structure support
  int timestamp;
};

#endif
