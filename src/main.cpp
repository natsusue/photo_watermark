#include <QtWidgets/QApplication>
#include <QGuiApplication>
#include <QDateTime>
#include "photo_watermark.h"
#include "mainWidgets.h"

void messageOutput(QtMsgType type, const QMessageLogContext & context, const QString & msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "[%s][Debug][[%s:%d][%s]%s\n", QDateTime::currentDateTime().toString().toUtf8().constData(),
                context.file, context.line, context.function, localMsg.constData());
        break;
    case QtInfoMsg:
        fprintf(stderr, "[%s][Info][[%s:%d][%s]%s\n", QDateTime::currentDateTime().toString().toUtf8().constData(),
                context.file, context.line, context.function, localMsg.constData());
        break;
    case QtWarningMsg:
        fprintf(stderr, "[%s][Warn][[%s:%d][%s]%s\n", QDateTime::currentDateTime().toString().toUtf8().constData(),
                context.file, context.line, context.function, localMsg.constData());
        break;
    case QtCriticalMsg:
        fprintf(stderr, "[%s][Critical][[%s:%d][%s]%s\n", QDateTime::currentDateTime().toString().toUtf8().constData(),
                context.file, context.line, context.function, localMsg.constData());
        break;
    case QtFatalMsg:
        fprintf(stderr, "[%s][Fatal][[%s:%d][%s]%s\n", QDateTime::currentDateTime().toString().toUtf8().constData(),
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
