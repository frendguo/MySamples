#pragma once

#include <windows.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <string>
#include <map>
#include <mutex>
#include <stdexcept>

#pragma comment(lib, "pdh.lib")

class PerformanceCounter {
public:
    // 构造函数
    PerformanceCounter();

    // 禁止复制构造和赋值
    PerformanceCounter(const PerformanceCounter&) = delete;
    PerformanceCounter& operator=(const PerformanceCounter&) = delete;

    // 移动构造和赋值
    PerformanceCounter(PerformanceCounter&& other) noexcept;
    PerformanceCounter& operator=(PerformanceCounter&& other) noexcept;

    // 添加计数器
    void AddCounter(const std::wstring& counterPath);

    // 移除计数器
    void RemoveCounter(const std::wstring& counterPath);

    // 收集数据
    void CollectData();

    // 获取所有计数器的值
    std::map<std::wstring, double> GetValues();

    // 析构函数
    ~PerformanceCounter();

private:
    HQUERY hQuery;                              // 查询句柄
    std::map<std::wstring, HCOUNTER> counterHandles; // 计数器路径与句柄的映射
    std::mutex mtx;                             // 用于线程安全

    void Close();                               // 关闭查询并释放资源
};
