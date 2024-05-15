#ifndef AVPLAYER_H
#define AVPLAYER_H

#include <QObject>
#include "Decoder.h"

extern "C"
{
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <libswresample/swresample.h>
#include <SDL.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
}

class YUV422Frame;

using AVTool::Decoder;

using FFrame = Decoder::FFrame;

class AVClock
{
public:
    AVClock()
        : m_pts(0.0),
        m_drift(0.0)
    {}

    inline void reset()
    {
        m_pts=0.0;
        m_drift=0.0;
    }

    inline void setClock(double pts) {
        setCloctAt(pts);
    }

    inline double getClock() {
        return m_drift+av_gettime_relative() / 1000000.0;
    }

private:
    inline void setCloctAt(double pts) {
        m_drift=pts-av_gettime_relative() / 1000000.0;
        m_pts=pts;
    }

    double m_pts;
    double m_drift;
};


class AVPlayer : public QObject
{
    Q_OBJECT

    friend void fillAStreamCallback(void* userData, uint8_t* stream, int len);
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
    void pause(bool isPause);

    AVPlayer::EPlayerState playeState();

    void clearPlayer();

private:
    int initSDL();
    void initVideo();

    void videoCallback();

    void initAVClock();

    double computeTargetDelay(double delay);
    double vpDuration(FFrame* lastFrame, FFrame* currentFrame);
    void displayImage(AVFrame* frame);

private:
    //解码器实例
    Decoder* m_decoder;
    AVFormatContext* m_fmtCtx;

    AVCodecParameters* m_audioCodecPar;
    SwrContext* m_swrCtx;
    uint8_t* m_audioBuf;

    uint32_t m_audioBufSize;
    uint32_t m_audioBufIndex;
    uint32_t m_duration;

    uint32_t m_lastAudPts;

    enum AVSampleFormat m_targetSampleFmt;

    //记录暂停前的时间
    double m_pauseTime;

    bool m_pause;

    AVFrame* m_audioFrame;

    int m_targetChannels;
    int m_targetFreq;
    int m_targetChannelLayout;
    int m_targetNbSamples;

    //音频播放时钟
    AVClock m_audioClock;

    //视频播放时钟
    AVClock m_videoClock;

    int m_volume;

    //同步时钟初始化标志，音视频异步线程
    //谁先读到初始标志位就由谁初始化时钟
    int m_clockInitFlag;

    int m_audioIndex;
    int m_videoIndex;

    int m_imageWidth;
    int m_imageHeight;
    float m_aspectRatio = 1; // 宽高比

    //终止标志
    int m_exit;

    double m_frameTimer;

    //延时时间
    double m_delay;

    AVCodecParameters* m_videoCodecPar;

    enum AVPixelFormat m_dstPixFmt;
    int m_swsFlags;
    SwsContext* m_swsCtx;

    uint8_t* m_buffer;

    uint8_t* m_pixels[4];
    int m_pitch[4];

signals:
    void AVTerminate();
    void AVPtsChanged(unsigned int pts);
    void AVDurationChanged(unsigned int duration);
    void frameChanged(QSharedPointer<YUV422Frame> frame);

public:
    inline void setVolume(int volumePer) { m_volume = (volumePer * SDL_MIX_MAXVOLUME / 100) % (SDL_MIX_MAXVOLUME + 1); };
    inline int getVolume() const { return m_volume; }
    inline float getAspectRatio() const { return m_aspectRatio; }
};

#endif // AVPLAYER_H
