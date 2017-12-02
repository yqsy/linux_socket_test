#include <stdio.h>

int main() {

  int c = 0;
  for (int i = 0; i < 100000000; ++i) {
    c = c + 1;
  }

  printf("%d", c);

  return 0;
}