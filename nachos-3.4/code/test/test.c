#include "syscall.h"


int main()
{
  int a = 1;
  a ++;
  Yield();
  Exit(a);
}
