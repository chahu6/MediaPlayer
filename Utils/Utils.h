#ifndef UTILS_H
#define UTILS_H

#include <chrono>
#include <QDir>

struct Utils
{
    static int64_t getCurTimestamp()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();
    }

    static bool mkDirs(QString dirPath)
    {
        QDir dir(dirPath);
        if(dir.exists())
        {
            return true;
        }
        else
        {
            bool result = dir.mkpath(dirPath);
            return result;
        }
    }

    static QString secondsToDurationStr(int seconds)
    {
        int h = seconds / 3600;
        int m = (seconds % 3600) / 60;
        int s = seconds % 60;

        QString h_s;
        if(h < 10)
        {
            h_s = QString("0%1").arg(h);
        }
        else
        {
            h_s = QString::number(h);
        }

        QString m_s;
        if(m<10)
        {
            m_s = QString("0%1").arg(m);
        }
        else
        {
            m_s = QString::number(m);
        }
        QString s_s;
        if(s<10)
        {
            s_s = QString("0%1").arg(s);
        }
        else
        {
            s_s = QString::number(s);
        }
        QString durationStr = QString("%1:%2:%3").arg(h_s).arg(m_s).arg(s_s);
        return durationStr;
    }
};

#endif // UTILS_H
