#ifndef PCB_H
#define PCB_H

#include "list.h"
#include "bitmap.h"

#define POF_MAXNUM 32

class POF;
class Thread;
class AddrSpace;

class PCB{
public:
  PCB(unsigned int inputPID);
  ~PCB();

  void RemoveChildPCB(PCB *inputPCB);
  void AppendChildPCB(PCB *inputPCB);
  void RemoveChildParentPCB();
  void RemoveParentPCBChild(int inputValue);
  PCB *GetChildPCB(int childPID);
  void PrintPCB();

  int FindPOFByName(char *filename);
  int GetNumFreePOF();
  int CreatePOF(int sofIndex, char *filename);
  void PrintPOF();
  void RemovePOF(int pofIndex);
  bool TestPOF(int pofIndex);
  void SetPOFoffset(int pofIndex, int of);
  int GetPOFoffset(int pofIndex);
  int GetPOFsofIndex(int pofIndex);

  int PID;
  Thread* thisThread;
  PCB* parentPCB;  
  List childrenPCB;
  int childExitValue;

  AddrSpace *thisAddrSpace;

  BitMap pofMap;
  POF *pofArray[POF_MAXNUM];
};
#endif
