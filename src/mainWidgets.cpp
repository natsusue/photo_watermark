#include "mainWidgets.h"
#include "utils.h"
#include <filesystem>
#include <iostream>
#include <ObjectArray.h>
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

static QStringList font_weight_items = {
    QStringLiteral("Thin"),
    QStringLiteral("ExtraLight"),
    QStringLiteral("Light"),
    QStringLiteral("Normal"),
    QStringLiteral("Medium"),
    QStringLiteral("DemiBold"),
    QStringLiteral("Bold"),
    QStringLiteral("ExtraBold"),
    QStringLiteral("Black")
};

mainWidgets::mainWidgets(QWidget * parent)
{
    ui_.setupUi(this);
    this->resize(800, 445);
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
    QFont f("MiSans Latin");
    ui_.fontComboBox->setCurrentFont(f);

    ui_.LTChoice->addItems(set_combox_items);
    ui_.LTChoice->setCurrentIndex(static_cast<int>(TextChoice::kModel));
    ui_.LTWeight->addItems(font_weight_items);
    ui_.LTWeight->setCurrentIndex(5);

    ui_.LBChoice->addItems(set_combox_items);
    ui_.LBChoice->setCurrentIndex(static_cast<int>(TextChoice::kLensModel));
    ui_.LBWeight->addItems(font_weight_items);
    ui_.LBWeight->setCurrentIndex(2);

    ui_.RTChoice->addItems(set_combox_items);
    ui_.RTChoice->setCurrentIndex(static_cast<int>(TextChoice::kExposureParam));
    ui_.RTWeight->addItems(font_weight_items);
    ui_.RTWeight->setCurrentIndex(5);

    ui_.RBChoice->addItems(set_combox_items);
    ui_.RBChoice->setCurrentIndex(static_cast<int>(TextChoice::kData));
    ui_.RBWeight->addItems(font_weight_items);
    ui_.RBWeight->setCurrentIndex(2);

    connect(ui_.LTChoice, &QComboBox::currentIndexChanged,
            this, [this](int x) { OnComboBoxChanged(x, ui_.LTEdit); });
    // 左下初始化
    connect(ui_.LBChoice, &QComboBox::currentIndexChanged,
            this, [this](int x) { OnComboBoxChanged(x, ui_.LBEdit); });
    // 右上初始化
    connect(ui_.RTChoice, &QComboBox::currentIndexChanged,
            this, [this](int x) { OnComboBoxChanged(x, ui_.RTEdit); });
    // 右下初始化
    connect(ui_.RBChoice, &QComboBox::currentIndexChanged,
            this, [this](int x) { OnComboBoxChanged(x, ui_.RBEdit); });

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
    p.auto_align = ui_.autoAlignCheckBox->checkState() == Qt::Checked;

    auto & lt_setting = p.text_settings[TextPosition::kLeftTop];
    lt_setting.text_type = static_cast<TextChoice>(ui_.LTChoice->currentIndex());
    lt_setting.weight = GetFontWeight(ui_.LTWeight);
    if (lt_setting.text_type == TextChoice::kCustomString)
        lt_setting.custom_data = ui_.LTEdit->text();

    auto & rt_setting = p.text_settings[TextPosition::kRightTop];
    rt_setting.text_type = static_cast<TextChoice>(ui_.RTChoice->currentIndex());
    rt_setting.weight = GetFontWeight(ui_.RTWeight);
    if (rt_setting.text_type == TextChoice::kCustomString)
        rt_setting.custom_data = ui_.RTEdit->text();

    auto & lb_setting = p.text_settings[TextPosition::kLeftBottom];
    lb_setting.text_type = static_cast<TextChoice>(ui_.LBChoice->currentIndex());
    lb_setting.weight = GetFontWeight(ui_.LBWeight);
    if (lb_setting.text_type == TextChoice::kCustomString)
        lb_setting.custom_data = ui_.LBEdit->text();

    auto & rb_setting = p.text_settings[TextPosition::kRightBottom];
    rb_setting.text_type = static_cast<TextChoice>(ui_.RBChoice->currentIndex());
    rb_setting.weight = GetFontWeight(ui_.RBWeight);
    if (rb_setting.text_type == TextChoice::kCustomString)
        rb_setting.custom_data = ui_.RBEdit->text();

    work_.Clean();
    if (!work_.Init(p, cb_))
    {
        QMessageBox::warning(this, QStringLiteral("处理失败"), QStringLiteral("请检查参数"));
        return;
    }

    work_.WorkStart();
    ui_.startButton->setEnabled(false);
}

void mainWidgets::OnInputBtnClick()
{
    QString dir_name = QFileDialog::getExistingDirectory(this, QStringLiteral("选择输入文件夹"),
                                                         ".", QFileDialog::DontUseNativeDialog);
    ui_.lnputEdit->setText(dir_name);
}

void mainWidgets::OnOutputBtnClick()
{
    QString dir_name = QFileDialog::getExistingDirectory(this, QStringLiteral("选择输出文件夹"),
                                                         ".", QFileDialog::DontUseNativeDialog);
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
    ui_.startButton->setEnabled(true);
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

int mainWidgets::GetFontWeight(const QComboBox * combo)
{
    QString string = combo->currentText();
    int size = static_cast<int>(font_weight_items.length());
    for (int i = 0; i < size; ++i)
    {
        if (font_weight_items[i] == string)
            return (i + 1) * 100;
    }
    bool ok = false;
    int weight = string.toInt(&ok);
    if (!ok)
        return 400;
    return weight;
}
