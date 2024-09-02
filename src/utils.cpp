#include "utils.h"

#if defined(WIN32) || defined(_WIN32)
#include <Windows.h>
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif
#elif defined(__linux__) || defined(_LINUX)
#include<limits.h>
#include<unistd.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

std::filesystem::path GetSelfPath()
{
    char path[PATH_MAX] = { 0 };
#if defined(WIN32) || defined(_WIN32)
    GetModuleFileNameA(nullptr, path, PATH_MAX);
    return path;
#elif defined(__linux__) || defined(_LINUX)
    ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
    return std::string(path, (count > 0)?count:0);
#elif defined(__APPLE__)
    // 未测试,没mac设备
    uint32_t bufsize = PATH_MAX;
    _NSGetExecutablePath(path, &bufsize);
    return std::string(path);
#endif
}
