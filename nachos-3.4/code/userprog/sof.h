#ifndef SOF_H
#define SOF_H

#include "string.h"
#include "synch.h"

class OpenFile;

class SOF{
 public:
  SOF(char *inputFilename, OpenFile *inputOpenFile);
  ~SOF();
  int Write(char *buffer, int size, int offset);
  int Read(char *buffer, int size, int offset);
  

  char *filename;
  OpenFile *openFile;
  unsigned int counter;
  Lock fileLock;
};

#endif
