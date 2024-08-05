#pragma once
#include "utils.h"
#include <atomic>
#include <filesystem>
#include <string>
#include <thread>
#include <functional>
#include <Qstring>
#include <map>
#include <QFont>

#include "exif.h"

using progress_callback = std::function<void(int cur, int failed, int total, bool done)>;

using WaterMarkParam = struct WaterMarkParam
{
    std::string input_path;
    std::string output_path;
    QFont font;
    double border_ratio = 0.02; // border pixel size = ImageWidth * border_ratio
    bool add_frame = true;
    std::string logo;
    TextChoice left_top_choice = TextChoice::kModel;
    std::string left_top_custom;
    TextChoice right_top_choice = TextChoice::kExposureParam;
    std::string right_top_custom;
    TextChoice left_bottom_choice = TextChoice::kLensModel;
    std::string left_bottom_custom;
    TextChoice right_bottom_choice = TextChoice::kData;
    std::string right_bottom_custom;
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

    QString genText(TextChoice & choice, easyexif::EXIFInfo & exif);

    void PaintLeftTop(QPainter * painter, QString & str, QFontMetrics * fm,
                      int source_height, int box_size) const;

    int PaintRightTop(QPainter * painter, QString & str, QFontMetrics * fm,
                      int width, int source_height, int box_size) const;

    void PaintLeftBottom(QPainter * painter, QString & str, QFontMetrics * fm,
                         int source_height, int ex_padding, int box_size) const;

    void PaintRightBottom(QPainter * painter, QString & str, QFontMetrics * fm,
                          int source_height, int ex_padding, int left_padding, int box_size) const;

    void PaintLogo(QPainter * painter, const QString & file_path,
                   int source_height, int font_box_height, int font_box_left, int box_size) const;

private:
    progress_callback cb_ = nullptr;
    WaterMarkParam param_ = { };
    std::filesystem::path self_dir_;

    std::vector<std::string> input_files_;
    std::map<std::string, std::string> logo_map_; // <make, file_path>

    std::atomic_bool working_ = { false };
    std::thread thread_;
};
