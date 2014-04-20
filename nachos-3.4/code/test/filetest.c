#include "syscall.h"

int main() {
  OpenFileId ofi, ofi2;
  char buffer[5];
  ofi = Open("test/filetestfile");
  ofi2 = Open("test/filetestfile2");
  Read(buffer, 5, ofi);
  Write(buffer, 5, ConsoleOutput);
  Read(buffer, 5, ofi);
  Write(buffer, 5, ConsoleOutput);
  Write(buffer,5, ofi2);
  Close(ofi);
  Close(ofi2);
  return 0;
}
