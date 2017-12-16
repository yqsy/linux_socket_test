#include <stdio.h>

#include <atomic>

void measure() {}

int main(int argc, char *argv[]) {

  if (argc < 3) {
    printf("Usage: \n %s hostname port\n %s -l port\n", argv[0], argv[0]);
    return 0;
  }

  return 0;
}