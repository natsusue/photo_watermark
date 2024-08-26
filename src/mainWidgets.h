#pragma once
#include "ui_mainWidgets.h"
#include "photo_watermark.h"
#include <QTextEdit>

class mainWidgets :public QWidget
{
    Q_OBJECT
public:
    mainWidgets(QWidget * parent = nullptr);

public slots:
    void OnStartBtnClick();
    void OnInputBtnClick();
    void OnOutputBtnClick();
    void OnComboBoxChanged(int index, QTextEdit * edit);

    void OnProcessComplete(int total, int failed);

signals:
    void processComplete(int total, int failed);

protected:
    void MainWidgetsProgressCallback(int cur, int failed, int total, bool done);
    bool eventFilter(QObject * object, QEvent * event);

    void setComboBoxTextAlignCenterAndBorderRadius(QComboBox * combo);
    static int GetFontWeight(const QComboBox * combo);

private:
    Ui::mainWidgets ui_;

    progress_callback cb_ = nullptr;

    PhotoWaterMarkWork work_;
};
