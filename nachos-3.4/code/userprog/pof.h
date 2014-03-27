#ifndef POF_H
#define POF_H

class POF{
 public:  
  POF(unsigned int inputIndex, char *inputFilename);
  ~POF();

  char *filename;
  int sofIndex;
  int offset;
};

#endif
