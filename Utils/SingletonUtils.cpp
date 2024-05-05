#include "SingletonUtils.h"

QAtomicPointer<SingletonUtils> SingletonUtils::m_instance = nullptr;
QMutex SingletonUtils::m_instance_mtx;

SingletonUtils::SingletonUtils(QObject *parent)
    : QObject{parent}
{}

SingletonUtils::~SingletonUtils()
{}

SingletonUtils *SingletonUtils::getInstance()
{
    if(m_instance.testAndSetOrdered(0,0))
    {
        QMutexLocker locker(&m_instance_mtx);
        m_instance.testAndSetOrdered(0, new SingletonUtils(nullptr));
    }
    return m_instance;
}

void SingletonUtils::setRecordDir(const QString &recordDir)
{
    m_recordDir = recordDir;
}

void SingletonUtils::setScreenSize(int width, int height)
{
    mScreenWidth = width;
    mScreenHeight = height;
}

int SingletonUtils::getScreenWidth()
{
    return mScreenWidth;
}

int SingletonUtils::getScreenHeight()
{
    return mScreenHeight;
}
