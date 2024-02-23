#pragma once
#include <string>
#include <Windows.h>
#include <iomanip>
#include <sstream>
#include <chrono>

std::string SystemTimeToFormattedString(const SYSTEMTIME& st) {
    // 将SYSTEMTIME转换为std::tm结构
    std::tm tm = {};
    tm.tm_year = st.wYear - 1900; // std::tm中的年份是从1900年开始计数的
    tm.tm_mon = st.wMonth - 1;    // std::tm中的月份是从0开始计数的
    tm.tm_mday = st.wDay;
    tm.tm_hour = st.wHour;
    tm.tm_min = st.wMinute;
    tm.tm_sec = st.wSecond;

    // 使用std::put_time格式化输出
    std::ostringstream oss;
    oss << std::put_time(&tm, "%m/%d/%Y %I:%M:%S %p");
    return oss.str();
}

std::string ConvertFileTimeIntToSystemTime(ULONGLONG fileTimeInt) {
    FILETIME fileTime;
    fileTime.dwLowDateTime = (DWORD)(fileTimeInt & 0xFFFFFFFF);
    fileTime.dwHighDateTime = (DWORD)(fileTimeInt >> 32);

    // 转换为SYSTEMTIME结构
    SYSTEMTIME systemTime;
    if (FileTimeToSystemTime(&fileTime, &systemTime)) {
        SystemTimeToTzSpecificLocalTime(NULL, &systemTime, &systemTime);
        return SystemTimeToFormattedString(systemTime);
    }

    return "";
}

std::string FormatSystemUpTime(long long uptimeNano100) {
    using namespace std::chrono;
    // 将100纳秒间隔转换为对应的秒数
    auto uptimeSeconds = duration_cast<seconds>(nanoseconds(uptimeNano100 * 100));

    // 计算天数、小时、分钟和秒数
    auto days = duration_cast<duration<int, std::ratio<86400>>>(uptimeSeconds);
    uptimeSeconds -= days;
    auto h = duration_cast<hours>(uptimeSeconds);
    uptimeSeconds -= h;
    auto m = duration_cast<minutes>(uptimeSeconds);
    uptimeSeconds -= m;
    auto s = duration_cast<seconds>(uptimeSeconds);

    // 使用字符串流格式化输出
    std::stringstream ss;
    ss << days.count() << " days ";
    ss << std::setfill('0') << std::setw(2) << h.count() << ":";
    ss << std::setfill('0') << std::setw(2) << m.count() << ":";
    ss << std::setfill('0') << std::setw(2) << s.count();
    ss << "." << std::setfill('0') << std::setw(3) << (uptimeNano100 % 10'000'000) / 10'000; // 计算毫秒部分

    return ss.str();
}

