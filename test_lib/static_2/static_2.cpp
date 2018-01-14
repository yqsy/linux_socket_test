#include <stdio.h>

void foo1();

void foo2()
{
  foo1();
  printf("foo2\n");
}