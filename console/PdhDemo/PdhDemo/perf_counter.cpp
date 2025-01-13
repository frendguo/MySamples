#include "perf_counter.h"
#include <stdexcept>
#include <iostream>

// 构造函数
PerformanceCounter::PerformanceCounter() : hQuery(nullptr) {
    PDH_STATUS status = PdhOpenQuery(NULL, 0, &hQuery);
    if (status != ERROR_SUCCESS) {
        throw std::runtime_error("Failed to open PDH query.");
    }
}

// 移动构造函数
PerformanceCounter::PerformanceCounter(PerformanceCounter&& other) noexcept
    : hQuery(other.hQuery), counterHandles(std::move(other.counterHandles)) {
    other.hQuery = nullptr;
}

// 移动赋值运算符
PerformanceCounter& PerformanceCounter::operator=(PerformanceCounter&& other) noexcept {
    if (this != &other) {
        Close();
        hQuery = other.hQuery;
        counterHandles = std::move(other.counterHandles);
        other.hQuery = nullptr;
    }
    return *this;
}

// 添加计数器
void PerformanceCounter::AddCounter(const std::wstring& counterPath) {
    std::lock_guard<std::mutex> lock(mtx);
    if (counterHandles.find(counterPath) != counterHandles.end()) {
        throw std::runtime_error("Counter already exists.");
    }

    HCOUNTER hCounter;
    PDH_STATUS status = PdhAddCounter(hQuery, counterPath.c_str(), 0, &hCounter);
    if (status != ERROR_SUCCESS) {
        throw std::runtime_error("Failed to add counter.");
    }

    counterHandles[counterPath] = hCounter;
}

// 移除计数器
void PerformanceCounter::RemoveCounter(const std::wstring& counterPath) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = counterHandles.find(counterPath);
    if (it == counterHandles.end()) {
        throw std::runtime_error("Counter not found.");
    }

    PdhRemoveCounter(it->second);
    counterHandles.erase(it);
}

// 收集数据
void PerformanceCounter::CollectData() {
    std::lock_guard<std::mutex> lock(mtx);
    PDH_STATUS status = PdhCollectQueryData(hQuery);
    if (status != ERROR_SUCCESS) {
        throw std::runtime_error("Failed to collect PDH data.");
    }
}

// 获取所有计数器的值
std::map<std::wstring, double> PerformanceCounter::GetValues() {
    std::lock_guard<std::mutex> lock(mtx);
    std::map<std::wstring, double> results;

    for (const auto& [path, hCounter] : counterHandles) {
        PDH_FMT_COUNTERVALUE counterValue;
        PDH_STATUS status = PdhGetFormattedCounterValue(hCounter, PDH_FMT_DOUBLE, NULL, &counterValue);
        if (status != ERROR_SUCCESS) {
            throw std::runtime_error("Failed to get value.");
        }
        results[path] = counterValue.doubleValue;
    }

    return results;
}

// 关闭查询并释放资源
void PerformanceCounter::Close() {
    if (hQuery) {
        PdhCloseQuery(hQuery);
        hQuery = nullptr;
    }
}

// 析构函数
PerformanceCounter::~PerformanceCounter() {
    Close();
}
