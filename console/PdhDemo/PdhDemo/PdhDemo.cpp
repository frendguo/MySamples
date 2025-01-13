#include "perf_counter.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    try {
        // 创建性能计数器对象
        PerformanceCounter counter;

        // 添加计数器
        counter.AddCounter(L"\\Processor(_Total)\\Interrupts/sec");
        counter.AddCounter(L"\\Processor(_Total)\\% Processor Time");
        counter.AddCounter(L"\\Memory\\Available MBytes");

        // !!! 这里一定要提前 collectdata
        counter.CollectData();
        Sleep(1000); // 等待 1 秒

        // 循环获取值
        for (int i = 0; i < 3; ++i) {
            counter.CollectData(); // 收集数据
            auto values = counter.GetValues(); // 获取值

            for (const auto& [path, value] : values) {
                std::wcout << path << L": " << value << std::endl;
            }

            std::wcout << L"-------------------------" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // 移除计数器
        counter.RemoveCounter(L"\\Processor(_Total)\\% Processor Time");
        std::wcout << L"Removed \\Processor(_Total)\\% Processor Time" << std::endl;

        // 再次获取值
        counter.CollectData();
        auto values = counter.GetValues();
        for (const auto& [path, value] : values) {
            std::wcout << path << L": " << value << std::endl;
        }

    }
    catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
