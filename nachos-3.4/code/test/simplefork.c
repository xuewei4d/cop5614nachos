#include "syscall.h"

void add() {
  int a, b, c;
  a = 1;
  b = 1;
  c = a + b;
  Exit(c);
}

int main () {
  int a, b, c;
  a = 2;
  b = 3;
  Fork(add);
  c = a*b;
  Exit(c);
}
