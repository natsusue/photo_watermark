#include "mainWidgets.h"
#include "utils.h"
#include <filesystem>
#include <iostream>
#include <QFileDialog>
#include <QMessageBox>

#include "photo_watermark.h"

static QStringList set_combox_items = {
    QStringLiteral("无"),
    QStringLiteral("相机型号"),
    QStringLiteral("镜头型号"),
    QStringLiteral("拍摄参数"),
    QStringLiteral("拍摄时间"),
    QStringLiteral("自定义字符串")
};

mainWidgets::mainWidgets(QWidget * parent)
{
    ui_.setupUi(this);
    this->resize(715, 400);
    connect(ui_.inputButton, &QPushButton::clicked, this, &mainWidgets::OnInputBtnClick);
    connect(ui_.outputButton, &QPushButton::clicked, this, &mainWidgets::OnOutputBtnClick);
    connect(ui_.startButton, &QPushButton::clicked, this, &mainWidgets::OnStartBtnClick);
    connect(this, &mainWidgets::processComplete, this, &mainWidgets::OnProcessComplete);

    // 初始配置初始化

    ui_.boxSizeComboBox->setValue(0.02);

    ui_.logoComboBox->addItem(QStringLiteral("Auto"));

    auto self_path = GetSelfPath().parent_path();
    std::filesystem::path logo_path = self_path / "logos";
    if (std::filesystem::exists(logo_path))
    {
        for (const auto & logo_file : std::filesystem::directory_iterator(logo_path))
            ui_.logoComboBox->addItem(QString::fromStdString(logo_file.path().stem().string()));
    }

    auto font_path(self_path / "font");
    if (std::filesystem::exists(font_path))
    {
        for (auto & file : std::filesystem::directory_iterator(font_path))
        {
            auto id = QFontDatabase::addApplicationFont(std::filesystem::absolute(file).string().c_str());
            if (id >= 0)
                QFontDatabase::applicationFontFamilies(id);
        }
    }
    qDebug() << QFontDatabase::families();
    QFont f("MiSans Latin");
    ui_.fontComboBox->setCurrentFont(f);

    // 左上初始化
    ui_.leftTopComboBox->addItems(set_combox_items);
    ui_.leftTopComboBox->setCurrentIndex(static_cast<int>(TextChoice::kModel));
    ui_.leftTopEdit->setText(QStringLiteral("例: \"Nikon Z9\" 若需要编辑请选择自定义字符串模式 "));
    ui_.leftTopEdit->setEnabled(false);
    connect(ui_.leftTopComboBox, &QComboBox::currentIndexChanged,
            this, [this](int x) { OnComboBoxChanged(x, ui_.leftTopEdit); });
    // 左下初始化
    ui_.leftBottomComboBox->addItems(set_combox_items);
    ui_.leftBottomComboBox->setCurrentIndex(static_cast<int>(TextChoice::kLensModel));
    ui_.leftBottomEdit->setText(QStringLiteral("例: \"Nikkor 58mm f/0.95\" 若需要编辑请选择自定义字符串模式 "));
    ui_.leftBottomEdit->setEnabled(false);
    connect(ui_.leftBottomComboBox, &QComboBox::currentIndexChanged,
            this, [this](int x) { OnComboBoxChanged(x, ui_.leftBottomEdit); });
    // 右上初始化
    ui_.rightTopcomboBox->addItems(set_combox_items);
    ui_.rightTopcomboBox->setCurrentIndex(static_cast<int>(TextChoice::kExposureParam));
    ui_.rightTopEdit->setText(QStringLiteral("例: \"58mm ISO100 1/100 f/1.2\" 若需要编辑请选择自定义字符串模式 "));
    ui_.rightTopEdit->setEnabled(false);
    connect(ui_.rightTopcomboBox, &QComboBox::currentIndexChanged,
            this, [this](int x) { OnComboBoxChanged(x, ui_.rightTopEdit); });
    // 右下初始化
    ui_.rightBottomComboBox->addItems(set_combox_items);
    ui_.rightBottomComboBox->setCurrentIndex(static_cast<int>(TextChoice::kData));
    ui_.rightBottomEdit->setText(QStringLiteral("若需要编辑请选择自定义字符串模式 "));
    ui_.rightBottomEdit->setEnabled(false);
    connect(ui_.rightBottomComboBox, &QComboBox::currentIndexChanged,
            this, [this](int x) { OnComboBoxChanged(x, ui_.rightBottomEdit); });

    cb_ = [this](int cur, int failed, int total, bool done)
    {
        MainWidgetsProgressCallback(cur, failed, total, done);
    };
}

void mainWidgets::OnStartBtnClick()
{
    WaterMarkParam p = { };
    p.input_path = ui_.lnputEdit->text().toStdString();
    p.output_path = ui_.outputEdit->text().toStdString();
    p.font = ui_.fontComboBox->currentFont();
    p.border_ratio = ui_.boxSizeComboBox->value();
    p.logo = ui_.logoComboBox->currentText().toStdString();
    p.add_frame = ui_.addFrameCheckBox->checkState() == Qt::Checked;

    p.left_top_choice = static_cast<TextChoice>(ui_.leftTopComboBox->currentIndex());
    if (p.left_top_choice == TextChoice::kCustomString)
        p.left_top_custom = ui_.leftTopEdit->text().toStdString();
    p.right_top_choice = static_cast<TextChoice>(ui_.rightTopcomboBox->currentIndex());
    if (p.right_top_choice == TextChoice::kCustomString)
        p.right_top_custom = ui_.rightTopEdit->text().toStdString();
    p.left_bottom_choice = static_cast<TextChoice>(ui_.leftBottomComboBox->currentIndex());
    if (p.left_bottom_choice == TextChoice::kCustomString)
        p.left_bottom_custom = ui_.leftBottomEdit->text().toStdString();
    p.right_bottom_choice = static_cast<TextChoice>(ui_.rightBottomComboBox->currentIndex());
    if (p.right_bottom_choice == TextChoice::kCustomString)
        p.right_bottom_custom = ui_.rightBottomEdit->text().toStdString();


    if (!work_.Init(p, cb_))
    {
        QMessageBox::warning(this, QStringLiteral("处理失败"), QStringLiteral("请检查参数"));
        return;
    }

    work_.WorkStart();
}

void mainWidgets::OnInputBtnClick()
{
    QString dir_name = QFileDialog::getExistingDirectory(this, QStringLiteral("选择输入文件夹"));
    ui_.lnputEdit->setText(dir_name);
}

void mainWidgets::OnOutputBtnClick()
{
    QString dir_name = QFileDialog::getExistingDirectory(this, QStringLiteral("选择输出文件夹"));
    ui_.outputEdit->setText(dir_name);
}

void mainWidgets::OnComboBoxChanged(int index, QLineEdit * edit)
{
    if (nullptr == edit)
        return;

    if (index == static_cast<int>(TextChoice::kCustomString))
    {
        edit->setEnabled(true);
        edit->clear();
    }
    else
        edit->setEnabled(false);
}

void mainWidgets::OnProcessComplete(int total, int failed)
{
    QString ss = QString::asprintf("处理完成\n共计处理%d张图片\n失败%d张", total, failed);
    QMessageBox::information(this, QStringLiteral("处理结果"), ss);
    ui_.workStatus->setText(QStringLiteral("处理未开始"));
}

void mainWidgets::MainWidgetsProgressCallback(int cur, int failed, int total, bool done)
{
    if (!done)
    {
        QString ss = QString::asprintf("处理中: %d/%d\t失败数: %d", cur, total, failed);
        ui_.workStatus->setText(ss);
        return;
    }
    processComplete(total, failed);
}
