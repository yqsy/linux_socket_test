#include <vector>

#include <stdio.h>

int main() {
  std::vector<char> vec;
  printf("%zd %zd\n", vec.size(), vec.capacity()); //  0 0

  vec.resize(1024);
  printf("%zd %zd\n", vec.size(), vec.capacity()); // 1024 1024

  // 以指数级别上涨的
  vec.resize(1300);
  printf("%zd %zd\n", vec.size(), vec.capacity()); // 1300 2048

  return 0;
}