#ifndef SINGLETONUTILS_H
#define SINGLETONUTILS_H

#include <QObject>
#include <QMutex>

class SingletonUtils : public QObject
{
    Q_OBJECT
public:
    explicit SingletonUtils(QObject *parent = nullptr);
    ~SingletonUtils();

public:
    static SingletonUtils* getInstance();

    void setRecordDir(const QString &recordDir);

    inline QString getRecordDir() const { return m_recordDir; }

    void setScreenSize(int width,int height);

    int getScreenWidth(); // 主屏幕分辨率宽
    int getScreenHeight(); // 主屏幕分辨率高

private:
    static QAtomicPointer<SingletonUtils> m_instance;
    static QMutex m_instance_mtx;

    QString m_recordDir;

    int mScreenWidth;
    int mScreenHeight;

signals:
};

#endif // SINGLETONUTILS_H
