#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include <iostream>
#include <chrono>
#include <thread>

void sig_hanadler(int singo)
{
    if (singo == SIGINT)
    {
        std::cout << "recv SIGINT and thread id: " << std::this_thread::get_id() << std::endl;
        exit(0);
    }
}

int main()
{
    std::cout << "main thread id: " << std::this_thread::get_id() << std::endl;

    if (signal(SIGINT, sig_hanadler) == SIG_ERR)
    {
        std::cout << "can't catch SIGINT\n";
    }

    sleep(1000);

    return 0;
}
