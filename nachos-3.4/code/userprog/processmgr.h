#ifndef PROCESSMGR_H
#define PROCESSMGR_H
#define PIDMAP_SIZE 3

#include "pcb.h"
#include "bitmap.h"
#include "synch.h"

class Lock;

class ProcessMgr{
 public:
  ProcessMgr();
  ~ProcessMgr();
  
  PCB *CreatePCB();
  void RemovePCB(PCB *inputPCB);
  int GetNumFreePCBs();
    
  BitMap pidMap;
  Lock  pidLock;
};
#endif
