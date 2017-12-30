#include <boost/circular_buffer.hpp>

#include <iostream>

int main()
{
  boost::circular_buffer<int> cbuf(300);

  for (int i = 0; i < 300; i++)
  {
    cbuf.push_back(i + 1);
  }
  // 1 2 3 4 5 6 .... 299 300

  for (int i = 0; i < 100; i++)
  {
    cbuf.push_back(i + 1 + 1000);
  }

  // 1001 1002 .... 101 102 ... 299 300

  for (size_t i = 0; i < cbuf.size(); i++)
  {
    std::cout << cbuf[i] << " ";
  }
  // 输出的是
  // 101 102 ... 299 300 ... 1001 ..1100
  // 证明是从最老的元素开始遍历的

  std::cout << std::endl;
  return 0;
}