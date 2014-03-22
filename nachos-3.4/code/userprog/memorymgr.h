#ifndef MEMORYMGR_H
#define MEMORYMGR_H

#include "bitmap.h"
#include "machine.h"
#include "synch.h"

class BitMap;
class Lock;

class MemoryMgr {
 public:
  MemoryMgr();                          // initialization
  ~MemoryMgr();
  int GetPage();                        // return first clear page; if not return -1
  void ClearPage(int pageNumber);            // reset page
  int GetNumFreePages();                 // number of free pages

 private:
  BitMap memoryMap;
  Lock memoryLock;
};

#endif
