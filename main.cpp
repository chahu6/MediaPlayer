#include "widget.h"

#include <QApplication>
#include <QsLog.h>
#include "Utils.h"

using namespace QsLogging;

void initLogger(const QString& logDir)
{
    if(Utils::mkDirs(logDir))
    {
        Logger& logger = Logger::instance();
        logger.setLoggingLevel(QsLogging::TraceLevel);
        //    QString timeStr = QDate::currentDate().toString("yyyy-MM-dd");
        //    QDateTime time = QDateTime::fromString(timeStr,"yyyy-MM-dd");
        QString logFile = QString("%1/run.log").arg(logDir);
        DestinationPtr des(
            DestinationFactory::MakeFileDestination(logFile,
                                                    EnableLogRotation,
                                                    MaxSizeBytes(1*1024*1024),
                                                    MaxOldLogCount(1)));
        logger.addDestination(des);
        QLOG_INFO() << "initLogger() success logDir=" << logDir;
    }
    else
    {
        QLOG_INFO() << "initLogger() error logDir=" << logDir;
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("Chahu666");
    QCoreApplication::setOrganizationDomain("https://gitee.com/chahu666");
    QCoreApplication::setApplicationName("MediaPlayer");
    QCoreApplication::setApplicationVersion("2.0");

    // 适配高分辨率
#if(QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Floor);
#elif(QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    QApplication a(argc, argv);

    // 设置样式
    QFile qssFile(":/res/QSS/widget.qss");
    if(qssFile.open(QFile::ReadOnly))
    {
        a.setStyleSheet(qssFile.readAll());
    }

    // 初始化日志库，必须在QApplication初始化之后，不然QApplication::applicationDirPath()没有值
    QString logDir = QApplication::applicationDirPath() + "/logs";
    initLogger(logDir);

    QString vInfo = QString("%1 V%2").arg(QCoreApplication::applicationName()).arg(QCoreApplication::applicationVersion());
    qDebug() << vInfo;
    QLOG_INFO() << vInfo;

    Widget w;
    w.show();
    return a.exec();
}
