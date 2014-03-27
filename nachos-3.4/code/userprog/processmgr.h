#ifndef PROCESSMGR_H
#define PROCESSMGR_H
#define PIDMAP_SIZE 3

#include "bitmap.h"
#include "synch.h"

class PCB;
class Lock;

class ProcessMgr{
 public:
  ProcessMgr(int size = PIDMAP_SIZE);
  ~ProcessMgr();
  
  PCB *CreatePCB();
  void RemovePCB(PCB *inputPCB);
  int GetNumFreePCBs();
  bool IsSet(int PID);
  PCB *GetPCB(int inputPID);

  int NumTotalPID;  
  BitMap pidMap;
  PCB **allPCB;
  Lock  pidLock;
  
};
#endif
