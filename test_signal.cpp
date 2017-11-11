#include <iostream>
#include <chrono>
#include <thread>

int main( )
{
    std::cout << "thread id: " << std::this_thread::get_id() << std::endl;
    
    return 0;
}
