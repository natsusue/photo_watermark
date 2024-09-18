#include "photo_watermark.h"
#include "exif.h"
#include "utils.h"

#include <iostream>
#include <filesystem>
#include <fstream>
#include <QImageReader>
#include <QPainter>
#include <QBuffer>
#include <QLabel>
#include <QVBoxLayout>

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
    ifs.seekg(0, std::ios::end);
    const qsizetype file_size = ifs.tellg();
    QByteArray image_data(file_size, 0);
    if (image_data.isNull())
    {
        qWarning() << "Create " << ifs.tellg() << " bytes buffer failed.";
        return false;
    }
    ifs.seekg(0, std::ios::beg);
    ifs.read(image_data.data(), file_size);
    ifs.close();

    easyexif::EXIFInfo exif;
    exif.clear();
    if (PARSE_EXIF_SUCCESS != exif.parseFrom(reinterpret_cast<unsigned char *>(image_data.data()),
                                             static_cast<unsigned>(file_size)))
    {
        qWarning() << "Parse " << image_path.c_str() << " exif failed.";
        return false;
    }

    QBuffer buffer;
    buffer.setData(image_data);
    buffer.open(QIODevice::ReadOnly);
    QImageReader image_reader(&buffer);
    image_reader.setAutoTransform(true);
    QImage source_img = image_reader.read();

    if (source_img.isNull())
    {
        qWarning() << "QImage open" << image_path.c_str() << "failed.";
        return false;
    }

    // 新建图片
    //TODO 若横竖比过大, 可能导致比例失调
    int border_size = static_cast<int>(static_cast<float>(
        std::max(source_img.width(), source_img.height()) * param_.border_ratio));
    int new_image_width = 0;
    int new_image_height = 0;
    int watermark_height = 4 * border_size;
    if (param_.add_frame)
    {
        new_image_width = static_cast<int>(source_img.width() + 2 * border_size);
        new_image_height = static_cast<int>(source_img.height() + watermark_height + border_size);
    }
    else
    {
        new_image_width = source_img.width();
        new_image_height = source_img.height() + watermark_height;
    }

    QImage img(new_image_width, new_image_height, QImage::Format_ARGB32);
    img.fill(QColor(255, 255, 255));

    QPainter img_painter;
    img_painter.begin(&img);
    if (param_.add_frame)
        img_painter.drawImage(border_size, border_size, source_img);
    else
        img_painter.drawImage(0, 0, source_img);

    int watermark_y = param_.add_frame ? border_size + source_img.height() : source_img.size().height();
    int left_draw_x = param_.add_frame ? 2 * border_size : border_size;

    img_painter.translate(0, watermark_y);
    PaintLeft(&img_painter, exif, left_draw_x, watermark_height, border_size);
    PaintRight(&img_painter, exif, new_image_width, watermark_height, border_size);
    img_painter.end();

    std::filesystem::path out_file(param_.output_path);
    out_file /= std::filesystem::path(image_path).filename();
    img.save(QString::fromLocal8Bit(out_file.string().data(), out_file.string().size()), "JPG", 100);

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

QString PhotoWaterMarkWork::genText(const TextType & choice, easyexif::EXIFInfo & exif) const
{
    QString ss;
    int num = static_cast<int>(1.0 / exif.ExposureTime);
    switch (choice)
    {
    case TextType::kModel:
        ss = QString(exif.Model.c_str());
        break;
    case TextType::kLensModel:
        ss = QString(exif.LensInfo.Model.c_str());
        break;
    case TextType::kExposureParam:
        ss = QString::asprintf("%dmm ISO%d 1/%d f/%.1lf", exif.FocalLengthIn35mm,
                               exif.ISOSpeedRatings, num, exif.FNumber);
        break;
    case TextType::kData:
        ss = QString(exif.DateTime.c_str());
        break;
    case TextType::kGps:
    {
        const auto & longitude = exif.GeoLocation.LonComponents;
        const auto & latitude = exif.GeoLocation.LatComponents;
        ss = QString::asprintf("%d°%d'%d\"%c %d°%d'%d\"%c",
                               static_cast<int>(latitude.degrees), static_cast<int>(latitude.minutes),
                               static_cast<int>(latitude.seconds), latitude.direction,
                               static_cast<int>(longitude.degrees), static_cast<int>(longitude.minutes),
                               static_cast<int>(longitude.seconds), longitude.direction);
        break;
    }
    default:
        return param_.auto_align ? "" : "&nbsp;";
    }
    if (ss.isEmpty())
    {
        ss = param_.auto_align ? "" : "&nbsp;";
    }
    return ss;
}

void PhotoWaterMarkWork::PaintLeft(QPainter * painter, easyexif::EXIFInfo & exif,
                                   int draw_x, int watermark_height, int board_size)
{
    const auto & lt = param_.text_settings[TextPosition::kLeftTop];
    const auto & lb = param_.text_settings[TextPosition::kLeftBottom];

    if (lt.text_type == TextType::kNone && lb.text_type == TextType::kNone)
        return;

    QString text;
    // LT
    if (lt.text_type != TextType::KRichText)
        text.append(QString(
            "<p style=';line-height:120%'><span style ='font-size:%1px; color:#323232; font-weight:%2'>%3</span>")\
            .arg(static_cast<int>(board_size * 0.7)).arg(lt.weight).arg(genText(lt.text_type, exif)));
    else
        text.append(lt.custom_data);
    // LB
    if (lb.text_type != TextType::KRichText)
        text.append(QString(
            "<p style=';line-height:120%'><span style ='font-size:%1px; color:#505050;font-weight:%2'>%3</span>")\
            .arg(static_cast<int>(board_size * 0.65)).arg(lb.weight).arg(genText(lb.text_type, exif)));
    else
        text.append(lb.custom_data);

    QTextDocument td;
    td.setDefaultFont(param_.font);
    td.setDefaultTextOption(QTextOption(Qt::AlignVCenter | Qt::AlignLeft));
    td.setHtml(text);
    QPoint to_point(draw_x, watermark_height / 2 - td.size().toSize().height() / 2);
    painter->translate(to_point);
    td.drawContents(painter);
    painter->translate(QPoint(0, 0) - to_point);
}

void PhotoWaterMarkWork::PaintRight(QPainter * painter, easyexif::EXIFInfo & exif,
                                    int image_width, int watermark_height, int board_size)
{
    const auto & rt = param_.text_settings[TextPosition::kRightTop];
    const auto & rb = param_.text_settings[TextPosition::kRightBottom];

    int draw_x = image_width - (param_.add_frame ? 2 * board_size : board_size);
    int text_height = 0;
    if (rt.text_type != TextType::kNone ||
        rb.text_type != TextType::kNone)
    {
        QString text;
        // RT
        if (rt.text_type != TextType::KRichText)
            text.append(QString("<p style='line-height:120%'><span style ='font-size:%1px; color:#323232; font-weight:%2'>%3</span>")\
                        .arg(static_cast<int>(board_size * 0.7)).arg(rt.weight).arg(genText(rt.text_type, exif)));
        else
            text.append(rt.custom_data);
        // RB
        if (rb.text_type != TextType::KRichText)
            text.append(QString("<p style='line-height:120%'><span style ='font-size:%1px; color:#505050;font-weight:%2'>%3</span>")\
                        .arg(static_cast<int>(board_size * 0.65)).arg(rb.weight).arg(genText(rb.text_type, exif)));
        else
            text.append(rt.custom_data);

        QTextDocument td;
        td.setDefaultFont(param_.font);
        td.setDefaultTextOption(QTextOption(Qt::AlignVCenter | Qt::AlignLeft));
        td.setHtml(text);
        draw_x -= td.size().toSize().width();
        text_height = td.size().toSize().height();
        QPoint to_point(draw_x, watermark_height / 2 - text_height / 2);
        painter->translate(to_point);
        td.drawContents(painter);
        painter->translate(QPoint(0, 0) - to_point);
    }

    PaintLogo(painter, exif, text_height, draw_x, board_size);
}

void PhotoWaterMarkWork::PaintLogo(QPainter * painter, easyexif::EXIFInfo & exif,
                                   int font_box_height, int font_box_left, int board_size)
{
    std::string logo_choice = exif.Make;
    if (param_.logo != "Auto" && !param_.logo.empty())
        logo_choice = param_.logo;
    if (logo_choice.empty())
        return;

    auto found = std::ranges::find_if(logo_map_,
                                      [&exif, &logo_choice](const std::pair<std::string, std::string> & ele)-> bool
                                      {
                                          return 0 == strncasecmp(logo_choice.c_str(), ele.first.c_str(),
                                                                  std::min(logo_choice.size(), ele.first.size()));
                                      });
    if (found == logo_map_.end())
        return;
    QImageReader image_reader(found->second.c_str());
    image_reader.setAutoTransform(true);
    QImage logo = image_reader.read();

    int img_h = font_box_height == 0 ? board_size * 2 : static_cast<int>(font_box_height * 0.9);
    int img_w = logo.scaledToHeight(img_h).width();
    int left_padding = font_box_left - 0.9 * board_size - img_w;
    int top_padding = 2 * board_size - img_h / 2;
    painter->drawImage(left_padding, top_padding, logo.scaled(img_w, img_h, Qt::KeepAspectRatio));
    // 如果右边没字就不画这根线
    if (font_box_height > 0)
    {
        painter->setPen(QPen(QColor(204, 204, 204), board_size * 0.1));
        painter->drawLine(font_box_left - board_size / 2, 2 * board_size - font_box_height * 0.52,
                          font_box_left - board_size / 2, 2 * board_size + font_box_height * 0.52);
    }
}
