#pragma once
#include "utils.h"
#include <atomic>
#include <filesystem>
#include <string>
#include <thread>
#include <functional>
#include <map>
#include <QFont>
#include <Qstring>

#include "exif.h"

using progress_callback = std::function<void(int cur, int failed, int total, bool done)>;

using TextPosition = enum class TextPosition
{
    kLeftTop,
    kRightTop,
    kLeftBottom,
    kRightBottom,
};

using TextSetting = struct TextSetting
{
    TextType text_type = TextType::kNone;
    QString custom_data;
    int weight = 0;
};

using WaterMarkParam = struct WaterMarkParam
{
    std::string input_path;
    std::string output_path;
    QFont font;
    double border_ratio = 0.02; // border pixel size = ImageWidth * border_ratio
    bool add_frame = true;
    bool auto_align = false;
    std::string logo;
    std::map<TextPosition, TextSetting> text_settings;
};

class PhotoWaterMarkWork
{
public:
    PhotoWaterMarkWork();

    bool Init(const WaterMarkParam & param, progress_callback cb);

    bool WorkStart();

    void Clean();

protected:
    void Work();

    bool ImageProcessing(const std::string & image_path);

    bool LoadLogos();

    QString genText(const TextType & choice, easyexif::EXIFInfo & exif) const;

    void PaintLeft(QPainter * painter, easyexif::EXIFInfo & exif,
                   int draw_x, int watermark_height, int board_size);

    void PaintRight(QPainter * painter, easyexif::EXIFInfo & exif,
                    int image_width, int watermark_height, int board_size);

    void PaintLogo(QPainter * painter, easyexif::EXIFInfo & exif,
                   int font_box_height, int font_box_left, int board_size);

private:
    progress_callback cb_ = nullptr;
    WaterMarkParam param_ = { };
    std::filesystem::path self_dir_;

    std::vector<std::string> input_files_;
    std::map<std::string, std::string> logo_map_; // <make, file_path>

    std::atomic_bool working_ = { false };
    std::thread thread_;
};
