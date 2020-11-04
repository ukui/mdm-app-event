#include <unistd.h>
#include <QApplication>

#include "mdm_app_event.h"

void messageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    static QMutex mutex;
    mutex.lock();

    QString text;
    switch (type)
    {
    case QtDebugMsg:
     text = QString("Debug:");
     break;

    case QtWarningMsg:
     text = QString("Warning:");
     break;

    case QtCriticalMsg:
     text = QString("Critical:");
     break;

    case QtFatalMsg:
     text = QString("Fatal:");
    }

    QString context_info = QString("File:(%1) Line:(%2)").arg(QString(context.file)).arg(context.line);
    QString current_date_time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss ddd");
    QString current_date = QString("(%1)").arg(current_date_time);
    QString message = QString("%1 %2 %3 %4").arg(text).arg(context_info).arg(current_date).arg(msg);

    QString logFilePath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/MdmAppEvent.log";
    // QString logFilePath = "/home/weinan1/prj/MdmAppEvent.log";
    QFile file(logFilePath);
    file.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream text_stream(&file);
    text_stream << message << "\r\n";
    file.flush();
    file.close();

    mutex.unlock();
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(messageOutput);
    //if (setuid(0) != 0)
    //    qWarning() << "enhance permission error"
    QApplication a(argc, argv);
    a.setSetuidAllowed(true);
    MdmAppEvent event;

    return a.exec();
}
