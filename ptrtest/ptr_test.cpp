#include <iostream>
#include <memory>
#include <thread>

int main() {
  std::shared_ptr<int> p1(new int);

  std::cout << "p1: " << p1.use_count() << std::endl;

  {
    std::shared_ptr<int> p2(p1);
    std::cout << "p2: " << p2.use_count() << " p1: " << p1.use_count() << "\n";
  }

  std::cout << "p1: " << p1.use_count() << std::endl;
  // std::cout << "p2: " << p2.use_count() << " p1: " << p1.use_count() << "\n";

  return 0;
}