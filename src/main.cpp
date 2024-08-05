#include <iostream>
#include <QtWidgets/QApplication>
#include <QGuiApplication>
#include "photo_watermark.h"
#include "mainWidgets.h"
#include "utils.h"
#include <chrono>
#include <ctime>

void messageOutput(QtMsgType type, const QMessageLogContext & context, const QString & msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    std::time_t tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    char * ctime = std::ctime(&tt);
    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "[%s][Debug][[%s:%d][%s]%s\n", ctime,
                context.file, context.line, context.function, localMsg.constData());
        break;
    case QtInfoMsg:
        fprintf(stderr, "[%s][Info][[%s:%d][%s]%s\n", ctime,
                context.file, context.line, context.function, localMsg.constData());
        break;
    case QtWarningMsg:
        fprintf(stderr, "[%s][Warn][[%s:%d][%s]%s\n", ctime,
                context.file, context.line, context.function, localMsg.constData());
        break;
    case QtCriticalMsg:
        fprintf(stderr, "[%s][Critical][[%s:%d][%s]%s\n", ctime,
                context.file, context.line, context.function, localMsg.constData());
        break;
    case QtFatalMsg:
        fprintf(stderr, "[%s][Fatal][[%s:%d][%s]%s\n", ctime,
                context.file, context.line, context.function, localMsg.constData());
        abort();
    }
}

int main(int argc, char * argv[])
{
	qInstallMessageHandler(messageOutput);
	QApplication a(argc,argv);
	mainWidgets main_widgets;
	main_widgets.show();
	a.exec();
	return 0;
}
