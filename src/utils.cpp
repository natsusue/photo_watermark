#include "utils.h"

#if defined(WIN32) || defined(_WIN32)
#include <Windows.h>
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif
#else
#include<unistd.h>
#endif

std::filesystem::path GetSelfPath()
{
    char path[PATH_MAX] = { 0 };
#if defined(WIN32) || defined(_WIN32)
    GetModuleFileNameA(nullptr, path, PATH_MAX);
    return path;
#else
    char result[PATH_MAX];     
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    return std::string(result, (count > 0)?count:0);
#endif
}