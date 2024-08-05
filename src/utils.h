#pragma once
#include <filesystem>

using TextChoice = enum class TextChoice
{
    kNone = 0,
    kModel,
    kLensModel,
    kExposureParam,
    kData,
    kCustomString,
    kMax
};

std::filesystem::path GetSelfPath();
