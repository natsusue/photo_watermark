#include "photo_watermark.h"
#include "exif.h"
#include "utils.h"

#include <iostream>
#include <filesystem>
#include <fstream>
#include <QImage>
#include <QImageReader>
#include <QPainter>
#include <QDebug>

#if defined(WIN32) || defined(_WIN32)
#ifndef strcasecmp
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif
#endif

PhotoWaterMarkWork::PhotoWaterMarkWork()
{
    const auto exe_path = GetSelfPath();
    self_dir_ = exe_path.parent_path();
}

bool PhotoWaterMarkWork::Init(const WaterMarkParam & param, progress_callback cb)
{
    if (working_)
    {
        qWarning() << "Processing, can't init again.";
        return false;
    }

    if (param.input_path.empty() || param.output_path.empty())
    {
        qWarning() << "Empty input path or output path";
        return false;
    }

    const std::filesystem::path input_dir(param.input_path);
    if (!std::filesystem::exists(input_dir))
    {
        qWarning() << "Input path :" << param.input_path.c_str() << " not exists.";
        return false;
    }

    const std::filesystem::path output_dir(param.output_path);
    if (!std::filesystem::exists(output_dir) && !std::filesystem::create_directory(output_dir))
    {
        qWarning() << "Create directory :" << output_dir.string().c_str() << "failed.";
        return false;
    }

    // get all input file
    for (const auto & file : std::filesystem::directory_iterator{ input_dir })
    {
        std::string ext = file.path().extension().string();
        if (0 == strcasecmp(ext.c_str(), ".jpg") || 0 == strcasecmp(ext.c_str(), ".jpeg"))
            input_files_.emplace_back(std::filesystem::absolute(file.path()).string());
    }

    if (input_files_.empty())
    {
        qWarning() << "Found valid image in path :" << param.input_path.c_str() << " failed.";
        return false;
    }

    if (!LoadLogos())
        qWarning() << "Load Logos failed.";

    param_ = param;
    cb_ = cb;
    return true;
}

bool PhotoWaterMarkWork::WorkStart()
{
    if (input_files_.empty())
    {
        qWarning() << "Empty input queue.";
        return false;
    }
    try
    {
        thread_ = std::thread(&PhotoWaterMarkWork::Work, this);
    }
    catch (const std::exception & e)
    {
        qWarning() << "Open work thread error:" << e.what();
    }

    return true;
}

void PhotoWaterMarkWork::Clean()
{
    if (working_)
        return;
    param_ = WaterMarkParam();
    input_files_.clear();
    logo_map_.clear();
    if (thread_.joinable())
        thread_.join();
}

void PhotoWaterMarkWork::Work()
{
    working_ = true;
    int total = input_files_.size();
    int cur = 0;
    int failed = 0;
    for (auto & file : input_files_)
    {
        ++cur;
        if (cb_)
            cb_(cur, failed, total, false);
        if (!ImageProcessing(file))
        {
            qWarning() << "Process file " << file.c_str() << "failed.";
            ++failed;
        }
    }
    if (cb_)
        cb_(cur, failed, total, true);
    working_ = false;
}

bool PhotoWaterMarkWork::ImageProcessing(const std::string & image_path)
{
    // 读取exif
    std::ifstream ifs(image_path, std::ios::in | std::ios::binary);
    if (!ifs.good())
    {
        qWarning() << "Open " << image_path.c_str() << "failed.";
        return false;
    }
    std::string image_data;
    ifs.seekg(0, std::ios::end);
    image_data.resize(ifs.tellg());
    ifs.seekg(0, std::ios::beg);
    ifs.read(&image_data[0], image_data.size());
    ifs.close();

    easyexif::EXIFInfo exif;
    exif.clear();
    if (PARSE_EXIF_SUCCESS != exif.parseFrom(image_data))
    {
        qWarning() << "Parse " << image_path.c_str() << " exif failed.";
        return false;
    }
    QImageReader image_reader(image_path.c_str());
    image_reader.setAutoTransform(true);
    QImage source_img = image_reader.read();

    if (source_img.isNull())
    {
        qWarning() << "QImage open" << image_path.c_str() << "failed.";
        return false;
    }

    // 新建图片
    //TODO 若横竖比过大, 可能导致比例失调
    int border_size = static_cast<int>(static_cast<float>(std::max(source_img.width(), source_img.height()) * param_.border_ratio));
    int new_image_width = 0;
    int new_image_height = 0;
    if (param_.add_frame)
    {
        new_image_width = static_cast<int>(source_img.width() + 2 * border_size);
        new_image_height = static_cast<int>(source_img.height() + 5 * border_size);
    }
    else
    {
        new_image_width = source_img.width();
        new_image_height = source_img.height() + 4 * border_size;
    }
    QImage img(new_image_width, new_image_height, QImage::Format_ARGB32);
    img.fill(QColor(255, 255, 255));

    QPainter img_painter;
    img_painter.begin(&img);
    if (param_.add_frame)
        img_painter.drawImage(border_size, border_size, source_img);
    else
        img_painter.drawImage(0, 0, source_img);

    // 第一行通用设置
    param_.font.setPixelSize(border_size * 0.75);
    param_.font.setBold(true);
    param_.font.setWeight(QFont::Medium);
    img_painter.setFont(param_.font);
    img_painter.setPen(QColor(33, 33, 33));
    QFontMetrics first_fm(param_.font);

    // 左上
    QString left_top_text;
    if (param_.left_top_choice != TextChoice::kCustomString)
        left_top_text = genText(param_.left_top_choice, exif);
    else
        left_top_text = QString::fromStdString(param_.left_top_custom);
    PaintLeftTop(&img_painter, left_top_text, &first_fm, source_img.height(), border_size);

    // 右上
    QString right_top_text;
    if (param_.right_top_choice != TextChoice::kCustomString)
        right_top_text = genText(param_.right_top_choice, exif);
    else
        right_top_text = QString::fromStdString(param_.right_top_custom);
    int r_left_padding = PaintRightTop(&img_painter, right_top_text, &first_fm,
                                       img.width(), source_img.height(), border_size);

    // 第二行通用设置
    param_.font.setPixelSize(border_size * 0.7);
    param_.font.setBold(false);
    param_.font.setWeight(QFont::Weight::Light);
    img_painter.setFont(param_.font);
    img_painter.setPen(QColor(114, 114, 114));
    QFontMetrics second_fm(param_.font);
    int ex_padding = 1.2 * first_fm.height();

    // 左下
    QString left_bottom_text;
    if (param_.left_bottom_choice != TextChoice::kCustomString)
        left_bottom_text = genText(param_.left_bottom_choice, exif);
    else
        left_bottom_text = QString::fromStdString(param_.left_bottom_custom);
    PaintLeftBottom(&img_painter, left_bottom_text, &second_fm, source_img.height(), ex_padding, border_size);

    // 右下
    QString right_bottom_text;
    if (param_.right_bottom_choice != TextChoice::kCustomString)
        right_bottom_text = genText(param_.right_bottom_choice, exif);
    else
        right_bottom_text = QString::fromStdString(param_.right_bottom_custom);
    PaintRightBottom(&img_painter, right_bottom_text, &second_fm,
                     source_img.height(), ex_padding, r_left_padding, border_size);

    // logo 和 线
    int font_height = ex_padding + second_fm.height();

    auto found = std::find_if(logo_map_.begin(), logo_map_.end(),
                              [&exif](const std::pair<std::string, std::string> & ele)-> bool
                              {
                                  return 0 == strncasecmp(exif.Make.c_str(), ele.first.c_str(),
                                                          std::min(exif.Make.size(), ele.first.size()));
                              });
    if (found != logo_map_.end())
        PaintLogo(&img_painter, found->second.c_str(), source_img.height(), font_height, r_left_padding, border_size);
    img_painter.end();

    std::filesystem::path out_file(param_.output_path);
    out_file /= std::filesystem::path(image_path).filename();
    img.save(QString::fromStdString(out_file.string()));

    return true;
}

bool PhotoWaterMarkWork::LoadLogos()
{
    const std::filesystem::path logos_path(self_dir_ / "logos");
    if (logos_path.empty() || !std::filesystem::exists(logos_path))
    {
        qWarning() << "Logos path " << logos_path.string().c_str() << "not exists.";
        return false;
    }

    for (const auto & file : std::filesystem::directory_iterator{ logos_path })
    {
        auto file_name = file.path().stem();
        logo_map_.emplace(file_name.string(), std::filesystem::absolute(file).string());
    }
    return true;
}

QString PhotoWaterMarkWork::genText(TextChoice & choice, easyexif::EXIFInfo & exif)
{
    QString ss;
    int num = static_cast<int>(1.0 / exif.ExposureTime);
    switch (choice)
    {
    case TextChoice::kModel:
        ss = QString(exif.Model.c_str());
        break;
    case TextChoice::kLensModel:
        ss = QString(exif.LensInfo.Model.c_str());
        break;
    case TextChoice::kExposureParam:
        ss = QString::asprintf("%dmm ISO%d 1/%d f/%.1lf", exif.FocalLengthIn35mm,
                               exif.ISOSpeedRatings, num, exif.FNumber);
        break;
    case TextChoice::kData:
        ss = QString(exif.DateTime.c_str());
    }
    return ss;
}

void PhotoWaterMarkWork::PaintLeftTop(QPainter * painter, QString & str, QFontMetrics * fm,
                                      int source_height, int box_size) const
{
    int left_padding = 0;
    int top_padding = 0;
    if (param_.add_frame)
    {
        left_padding = box_size * 2;
        top_padding = 2 * box_size + source_height;
    }
    else
    {
        left_padding = box_size;
        top_padding = box_size + source_height;
    }
    QRect r = fm->boundingRect(QRect(0, 0, 0, 0), Qt::AlignLeft | Qt::AlignTop, str);
    r.setRect(left_padding, top_padding, r.size().width(), r.size().height());
    painter->drawText(r, Qt::AlignLeft | Qt::AlignTop, str);
}

int PhotoWaterMarkWork::PaintRightTop(QPainter * painter, QString & str, QFontMetrics * fm,
                                      int width, int source_height, int box_size) const
{
    int left_padding = 0;
    int top_padding = 0;
    if (param_.add_frame)
    {
        left_padding = width - 2 * box_size;
        top_padding = 2 * box_size + source_height;
    }
    else
    {
        left_padding = width - box_size;
        top_padding = box_size + source_height;
    }

    QRect r = fm->boundingRect(QRect(0, 0, 0, 0), Qt::AlignLeft | Qt::AlignTop, str);
    left_padding -= r.width();
    r.setRect(left_padding, top_padding, r.size().width(), r.size().height());
    painter->drawText(r, Qt::AlignLeft | Qt::AlignTop, str);
    return left_padding;
}

void PhotoWaterMarkWork::PaintLeftBottom(QPainter * painter, QString & str, QFontMetrics * fm, int source_height,
                                         int ex_padding, int box_size) const
{
    int left_padding = 0;
    int top_padding = 0;

    if (param_.add_frame)
    {
        left_padding = 2 * box_size;
        top_padding = 2 * box_size + source_height + ex_padding;
    }
    else
    {
        left_padding = box_size;
        top_padding = box_size + source_height + ex_padding;
    }

    QRect r = fm->boundingRect(QRect(0, 0, 0, 0), Qt::AlignLeft | Qt::AlignTop, str);
    r.setRect(left_padding, top_padding, r.size().width(), r.size().height());
    painter->drawText(r, Qt::AlignLeft | Qt::AlignTop, str);
}

void PhotoWaterMarkWork::PaintRightBottom(QPainter * painter, QString & str, QFontMetrics * fm, int source_height,
                                          int ex_padding, int left_padding, int box_size) const
{
    int top_padding = 2 * box_size + source_height + ex_padding;
    if (param_.add_frame)
        top_padding = 2 * box_size + source_height + ex_padding;
    else
        top_padding = box_size + source_height + ex_padding;

    QRect r = fm->boundingRect(QRect(0, 0, 0, 0), Qt::AlignLeft | Qt::AlignTop, str);
    r.setRect(left_padding, top_padding, r.size().width(), r.size().height());
    qDebug() << "Right Bottom" << r;
    painter->drawText(r, Qt::AlignLeft | Qt::AlignTop, str);
}

void PhotoWaterMarkWork::PaintLogo(QPainter * painter, const QString & file_path,
                                   int source_height, int font_box_height, int font_box_left, int box_size) const
{
    QImageReader image_reader(file_path);
    image_reader.setAutoTransform(true);
    QImage img = image_reader.read();

    int img_h = static_cast<int>(font_box_height * 0.85);
    int img_w = img.scaledToHeight(img_h).width();
    int top_padding = 0;
    int left_padding = font_box_left - 0.9 * box_size - img_w;

    if (param_.add_frame)
        top_padding = source_height + (2 * box_size) + (font_box_height / 2) - (img_h / 2);
    else
        top_padding = source_height + box_size + (font_box_height / 2) - (img_h / 2);
    painter->drawImage(left_padding, top_padding, img.scaled(img_w, img_h, Qt::KeepAspectRatio));

    painter->setPen(QPen(QColor(204, 204, 204), box_size * 0.1));
    int y = 0;
    if (param_.add_frame)
        y = source_height + 2 * box_size;
    else
        y = source_height + box_size;

    painter->drawLine(font_box_left - 0.5 * box_size, y - font_box_height * 0.1,
                      font_box_left - 0.5 * box_size, y + font_box_height * 1.1);
}
