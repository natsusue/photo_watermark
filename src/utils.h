#pragma once
#include <filesystem>

using TextType = enum class TextType
{
    kNone = 0,
    kModel,
    kLensModel,
    kExposureParam,
    kData,
    kGps,
    kCustomString,
    KRichText,
    kMax
};

std::filesystem::path GetSelfPath();
