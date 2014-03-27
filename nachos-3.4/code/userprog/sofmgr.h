#ifndef SOFMGR_H
#define SOFMGR_H

#include "bitmap.h"
#define SOF_MAXNUM 128

class SOF;
class SOFMgr{
 public:
  SOFMgr();
  ~SOFMgr();
  int FindSOFByName(char *filename);
  unsigned int GetNumFreeSOF() { return sofMap.NumClear(); }
  int CreateSOF(char *filename);
  void PrintSOF();
  void IncSOFCounter(int sofIndex);
  void DecSOFCounter(int sofIndex);
  void RemoveSOF(int sofIndex);
  int WriteSOF(char *buffer, int size, int offset, int sofIndex);
  int ReadSOF(char *buffer, int size, int offset, int sofIndex);
  
  BitMap sofMap;
  SOF *sofArray[SOF_MAXNUM];
};

#endif
