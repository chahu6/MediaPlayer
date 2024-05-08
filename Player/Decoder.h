#ifndef DECODER_H
#define DECODER_H

#include <QVector>
#include <condition_variable>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace AVTool
{
    class Decoder
    {
    public:
        typedef struct FFrame
        {
            AVFrame frame;
            int serial;
            double duration;
            double pts;
        }FFrame;

    private:
        typedef struct FPacket
        {
            AVPacket pkt;
            int serial;
        }FPacket;

        typedef struct FPacketQueue
        {
            QVector<FPacket> pktVec;
            int readIndex;
            int pushIndex;
            int size;
            int serial;
            std::mutex mutex;
            std::condition_variable cond;
        }FPacketQueue;

        typedef struct FFrameQueue
        {
            QVector<FFrame> frameVec;
            int readIndex;
            int pushIndex;
            int shown;
            int size;
            std::mutex mutex;
            std::condition_variable cond;
        }FFrameQueue;

        typedef struct FPktDecoder
        {
            AVCodecContext* codecCtx = nullptr; // 解码器上下文
            int serial;
        }FPktDecoder;

    public:
        Decoder();
        ~Decoder();

        bool decode(const QString& url);

    private:
        void init();

        void exit();

        void setInitVal();

        void demux();

        void audioDecode();
        void videoDecode();

    private:
        FPacketQueue m_audioPacketQueue;
        FPacketQueue m_videoPacketQueue;

        FFrameQueue m_audioFrameQueue;
        FFrameQueue m_videoFrameQueue;

        FPktDecoder m_audioPktDecoder;
        FPktDecoder m_videoPktDecoder;

        const int m_maxFrameQueueSize;
        const int m_maxPacketQueueSize;

        AVFormatContext* m_pAvFormatCtx = nullptr; // 流文件解析上下文

        std::atomic_bool m_exit;

        char m_errBuf[100];

        AVRational m_vidFrameRate;

        uint32_t m_audioIndex = -1; // 音频流索引值
        uint32_t m_videoIndex = -1; // 视频流索引值

        //是否执行跳转
        int m_isSeek;

        //跳转后等待目标帧标志
        int m_vidSeek;
        int m_audSeek;

        //跳转相对时间
        int64_t m_seekRel;

        //跳转绝对时间
        int64_t m_seekTarget;

        //流总时长/S
        uint32_t m_duration;
    };
}

#endif // DECODER_H
