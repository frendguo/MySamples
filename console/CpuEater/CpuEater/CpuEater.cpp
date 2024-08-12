// CpuEater.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <thread>
#include <Windows.h>

int main()
{
    std::cout << "CpuEater is running!\n";

    std::cout << "Please enter the number of threads you want to run: ";
    int numThreads;
    std::cin >> numThreads;

    std::cout << "Please enter the number of seconds you want to run the threads: ";
    int numSeconds;
    std::cin >> numSeconds;

    for (int i = 0; i < numThreads; i++)
    {
        std::thread t([](int numSeconds) {
            auto start = std::chrono::high_resolution_clock::now();
            while (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start).count() < numSeconds)
            {
                // Do nothing
            }
            }, numSeconds);
        t.detach();
    }

    int count = 0;
    while (true)
    {
        // 假的计时
        Sleep(1000);
        count++;
        std::cout << "run time: " << count << "s\n";
        if (count >= numSeconds) {
            break;
        }
    }

    system("pause");
}