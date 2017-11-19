#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int g1 = 1;

int main() {

  fork();

  // 2^1 会输出2次
  printf("hello %d\n", g1++);

  fork();
  fork();

  // 2^3 会输出8次
  printf("world %d\n", g1++);
  return 0;
}
