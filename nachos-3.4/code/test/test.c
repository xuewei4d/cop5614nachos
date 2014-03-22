#include "syscall.h"

void add() {
  Exit(222);
}
int main()
{
  Fork(add);
  Exit(999);
}
