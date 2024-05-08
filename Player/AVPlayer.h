#ifndef AVPLAYER_H
#define AVPLAYER_H

#include <QObject>

namespace AVTool {
    class Decoder;
}
class AVFormatContext;

class AVPlayer : public QObject
{
    Q_OBJECT
public:
    enum EPlayerState
    {
        AV_STOPPED,
        AV_PLAYING,
        AV_PAUSED
    };

    explicit AVPlayer(QObject *parent = nullptr);
    ~AVPlayer();

    bool play(const QString& url);

    AVPlayer::EPlayerState playeState();

private:
    void clearPlayer();

private:
    //解码器实例
    AVTool::Decoder* m_decoder;
    AVFormatContext* m_fmtCtx;

signals:
};

#endif // AVPLAYER_H
