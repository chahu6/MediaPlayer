#include "Decoder.h"

#include <QsLog.h>
#include "ThreadPool.h"

AVTool::Decoder::Decoder()
    : m_maxFrameQueueSize(16),
      m_maxPacketQueueSize(30),
      m_videoIndex(-1),
      m_audioIndex(-1),
      m_duration(0),
      m_pAvFormatCtx(nullptr),
      m_exit(0)
{
    init();
}

AVTool::Decoder::~Decoder()
{
    this->exit();
}

bool AVTool::Decoder::decode(const QString &url)
{
    int ret = 0;
    // 解封装初始化
    m_pAvFormatCtx = avformat_alloc_context();

    // 用于获取流时长
    AVDictionary* formatOpts = nullptr;
    av_dict_set(&formatOpts, "probesize", "32", 0);

    // 打开媒体文件
    ret = avformat_open_input(&m_pAvFormatCtx, url.toUtf8().constData(), nullptr, nullptr);
    if(ret < 0)
    {
        av_strerror(ret, m_errBuf, sizeof(m_errBuf));
        QLOG_ERROR() << "avformat_open_input error: " << m_errBuf;
        av_dict_free(&formatOpts);
        return false;
    }

    // 查找所有媒体流信息
    ret = avformat_find_stream_info(m_pAvFormatCtx, nullptr);
    if(ret < 0)
    {
        av_strerror(ret, m_errBuf, sizeof(m_errBuf));
        QLOG_ERROR() << "avformat_find_stream_info error:" << m_errBuf;
        av_dict_free(&formatOpts);
        return false;
    }

    // 记录流时长
    // 你定义了一个 AVRational 结构体 q，其分子为 1，分母为 AV_TIME_BASE。AV_TIME_BASE 是一个宏，通常定义为 1000000（即1秒等于1,000,000个单位），用于表示时间的基本单位。因此，q 表示的是从微秒（或更小的单位，取决于 AV_TIME_BASE 的实际值）到秒的转换比例。
    // 2. av_q2d 函数：
    // av_q2d 是 FFmpeg 中的一个函数，用于将 AVRational 结构体转换为一个双精度浮点数（double）。这是通过将 AVRational 的 num 除以 den 来实现的。
    AVRational q = {1, AV_TIME_BASE};
    m_duration = (uint32_t)(m_pAvFormatCtx->duration * av_q2d(q));
    av_dict_free(&formatOpts);

    // 找出音频对应的流的下标
    m_audioIndex = av_find_best_stream(m_pAvFormatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if(m_audioIndex < 0)
    {
        QLOG_ERROR() << "no audio stream";
        return false;
    }

    m_videoIndex = av_find_best_stream(m_pAvFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (m_videoIndex < 0)
    {
        QLOG_ERROR() << "no video stream!";
        return false;
    }

    //音频解码初始化
    // 创建音频解码器并且打开
    AVCodecParameters* audioCodecPar = m_pAvFormatCtx->streams[m_audioIndex]->codecpar;
    if(!audioCodecPar)
    {
        QLOG_ERROR() << "audio par is nullptr";
        return false;
    }

    m_audioPktDecoder.codecCtx = avcodec_alloc_context3(nullptr);

    ret = avcodec_parameters_to_context(m_audioPktDecoder.codecCtx, audioCodecPar);
    if(ret < 0)
    {
        av_strerror(ret, m_errBuf, sizeof(m_errBuf));
        QLOG_ERROR() << "avcodec_parameters_to_context error: " << m_errBuf;
        return false;
    }

    const AVCodec* audioCodec = avcodec_find_decoder(m_audioPktDecoder.codecCtx->codec_id);
    if(!audioCodec)
    {
        QLOG_ERROR() << "avcodec_find_decoder failed";
        return false;
    }
    // 这里应该不用在赋值了，多余了
    m_audioPktDecoder.codecCtx->codec_id = audioCodec->id;

    // 如果上面的avcodec_alloc_context3的参数已经赋值过了，就不需要在这里填第二个参数了，直接nullptr
    ret = avcodec_open2(m_audioPktDecoder.codecCtx, audioCodec, nullptr);
    if(ret < 0)
    {
        av_strerror(ret, m_errBuf, sizeof(m_errBuf));
        QLOG_ERROR() << "error info_avcodec_open2:" << m_errBuf;
        return false;
    }

    //视频解码初始化
    // 创建视频解码器并且打开
    AVCodecParameters* videoCodecPar= m_pAvFormatCtx->streams[m_videoIndex]->codecpar;
    if (!videoCodecPar) {
        QLOG_ERROR() << "videocodecpar is nullptr!";
        return false;
    }

    m_videoPktDecoder.codecCtx = avcodec_alloc_context3(nullptr);

    ret = avcodec_parameters_to_context(m_videoPktDecoder.codecCtx, videoCodecPar);
    if(ret < 0)
    {
        av_strerror(ret, m_errBuf, sizeof(m_errBuf));
        QLOG_ERROR() << "avcodec_parameters_to_context error: " << m_errBuf;
        return false;
    }

    // 找到视频解码器
    const AVCodec* videoCodec = avcodec_find_decoder(m_videoPktDecoder.codecCtx->codec_id);
    if(!videoCodec)
    {
        QLOG_ERROR() << "avcodec_find_decoder failed!";
        return false;
    }
    // 这里应该不用在赋值了，多余了
    m_videoPktDecoder.codecCtx->codec_id = videoCodec->id;

    ret = avcodec_open2(m_videoPktDecoder.codecCtx, videoCodec, nullptr);
    if(ret < 0)
    {
        av_strerror(ret, m_errBuf, sizeof(m_errBuf));
        QLOG_ERROR() << "error info_avcodec_open2:" << m_errBuf;
        return false;
    }

    // 记录视频帧率
    m_vidFrameRate = av_guess_frame_rate(m_pAvFormatCtx, m_pAvFormatCtx->streams[m_videoIndex], nullptr);

    setInitVal();

    ThreadPool::instance().commit([this](){
        this->demux();
    });
    ThreadPool::instance().commit([this](){
        this->audioDecode();
    });
    ThreadPool::instance().commit([this](){
        this->videoDecode();
    });

    return true;
}

void AVTool::Decoder::init()
{
    ThreadPool::instance();

    m_audioPacketQueue.pktVec.resize(m_maxPacketQueueSize);
    m_videoPacketQueue.pktVec.resize(m_maxPacketQueueSize);

    m_audioFrameQueue.frameVec.resize(m_maxFrameQueueSize);
    m_videoFrameQueue.frameVec.resize(m_maxFrameQueueSize);

    m_audioPktDecoder.codecCtx = nullptr;
    m_videoPktDecoder.codecCtx = nullptr;
}

void AVTool::Decoder::exit()
{
    m_exit.store(true);
}

void AVTool::Decoder::setInitVal()
{
    m_audioPacketQueue.size=0;
    m_audioPacketQueue.pushIndex=0;
    m_audioPacketQueue.readIndex=0;
    m_audioPacketQueue.serial=0;

    m_videoPacketQueue.size=0;
    m_videoPacketQueue.pushIndex=0;
    m_videoPacketQueue.readIndex=0;
    m_videoPacketQueue.serial=0;

    m_audioFrameQueue.size=0;
    m_audioFrameQueue.readIndex=0;
    m_audioFrameQueue.pushIndex=0;
    m_audioFrameQueue.shown=0;

    m_videoFrameQueue.size=0;
    m_videoFrameQueue.readIndex=0;
    m_videoFrameQueue.pushIndex=0;
    m_videoFrameQueue.shown=0;

    m_exit = 0;

    m_isSeek=0;

    m_audSeek=0;
    m_vidSeek=0;

    m_audioPktDecoder.serial=0;
    m_videoPktDecoder.serial=0;
}

void AVTool::Decoder::demux()
{
    int ret = -1;
    AVPacket* pkt = av_packet_alloc();
    while(true)
    {
        if(m_exit.load()) break;

        if(m_audioPacketQueue.size >= m_maxPacketQueueSize || m_videoPacketQueue.size >= m_maxPacketQueueSize)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }
        if(m_isSeek)
        {

        }
    }
}

void AVTool::Decoder::audioDecode()
{
    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    while(true)
    {
        if(m_exit.load()) break;
        // 音频帧队列长度控制
        if(m_audioFrameQueue.size >= m_maxFrameQueueSize)
        {

        }
    }
}

void AVTool::Decoder::videoDecode()
{

}
