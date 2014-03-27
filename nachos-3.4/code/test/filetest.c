#include "syscall.h"

int main() {
  OpenFileId ofi;
  char buffer[5];
  ofi = Open("test/filetestfile");
  Read(buffer, 5, ofi);
  Write(buffer, 5, ConsoleOutput);
  Close(ofi);
  return 0;
}
