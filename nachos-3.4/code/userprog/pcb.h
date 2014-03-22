#ifndef PCB_H
#define PCB_H

#include "list.h"
#include "machine.h"

class Thread;

class PCB{
public:
  PCB(unsigned int inputPID);
  ~PCB();

  void RemoveChildPCB(PCB *inputPCB);
  void AppendChildPCB(PCB *inputPCB);
  void RemoveChildParentPCB();
  void RemoveParentPCBChild(int inputValue);
  void PrintPCB();

  int PID;
  Thread* thisThread;
  PCB* parentPCB;  
  List childrenPCB;
  int childExitValue;
};
#endif
