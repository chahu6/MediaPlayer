#ifndef AVPLAYER_H
#define AVPLAYER_H

#include <QObject>

class AVPlayer : public QObject
{
    Q_OBJECT
public:
    explicit AVPlayer(QObject *parent = nullptr);

    bool play(const QString& url);

private:
    void clearPlayer();

signals:
};

#endif // AVPLAYER_H
