#include "Decoder.h"

#include <QsLog.h>
#include "ThreadPool.h"

AVTool::Decoder::Decoder()
    : m_maxFrameQueueSize(16),
      m_maxPacketQueueSize(30),
      m_pAvFormatCtx(nullptr),
      m_exit(0),
      m_audioIndex(-1),
      m_videoIndex(-1),
      m_duration(0)
{
    init();
    setInitVal(); // 初始化数据, 这里必须初始化，不然销毁时，size会有随机值，不是0
}

AVTool::Decoder::~Decoder()
{
    this->exit();
}

bool AVTool::Decoder::decode(const QString &url)
{
    setInitVal(); // 初始化数据

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
    if(m_exit.load()) return;

    m_exit.store(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    clearQueueCache();
    if(m_pAvFormatCtx != nullptr)
    {
        avformat_close_input(&m_pAvFormatCtx);
        m_pAvFormatCtx = nullptr;
    }
    if(m_audioPktDecoder.codecCtx != nullptr) {
        avcodec_free_context(&m_audioPktDecoder.codecCtx);
        m_audioPktDecoder.codecCtx=nullptr;
    }
    if(m_videoPktDecoder.codecCtx != nullptr) {
        avcodec_free_context(&m_videoPktDecoder.codecCtx);
        m_videoPktDecoder.codecCtx=nullptr;
    }
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

    m_exit.store(false);

    m_isSeek=0;

    m_audSeek=0;
    m_vidSeek=0;

    m_audioPktDecoder.serial=0;
    m_videoPktDecoder.serial=0;
}

void AVTool::Decoder::clearQueueCache()
{
    {
        std::lock_guard<std::mutex> lockAP(m_audioPacketQueue.mutex);

        while(m_audioPacketQueue.size > 0)
        {
            av_packet_unref(&m_audioPacketQueue.pktVec[m_audioPacketQueue.readIndex].pkt);
            m_audioPacketQueue.readIndex = (m_audioPacketQueue.readIndex + 1) % m_maxPacketQueueSize;
            m_audioPacketQueue.size--;
        }
    }

    {
        std::lock_guard<std::mutex> lockVP(m_videoPacketQueue.mutex);

        while(m_videoPacketQueue.size > 0)
        {
            av_packet_unref(&m_videoPacketQueue.pktVec[m_videoPacketQueue.readIndex].pkt);
            m_videoPacketQueue.readIndex = (m_videoPacketQueue.readIndex + 1) % m_maxPacketQueueSize;
            m_videoPacketQueue.size--;
        }
    }

    {
        std::lock_guard<std::mutex> lockAF(m_audioFrameQueue.mutex);

        while(m_audioFrameQueue.size > 0) {
            av_frame_unref(&m_audioFrameQueue.frameVec[m_audioFrameQueue.readIndex].frame);
            m_audioFrameQueue.readIndex = (m_audioFrameQueue.readIndex + 1) % m_maxFrameQueueSize;
            m_audioFrameQueue.size--;
        }
    }

    {
        std::lock_guard<std::mutex> lockVF(m_videoFrameQueue.mutex);

        while(m_videoFrameQueue.size > 0) {
            av_frame_unref(&m_videoFrameQueue.frameVec[m_videoFrameQueue.readIndex].frame);
            m_videoFrameQueue.readIndex = (m_videoFrameQueue.readIndex + 1) % m_maxFrameQueueSize;
            m_videoFrameQueue.size--;
        }
    }
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

        // 读取音视频数据包进行解码处理
        ret = av_read_frame(m_pAvFormatCtx, pkt);
        if (ret == AVERROR_EOF)  // 正常读取到文件尾部退出
        {
            av_packet_free(&pkt);
            QLOG_INFO() << "已到达媒体文件结尾";
            break;
        }
        else if(ret < 0)
        {
            av_packet_free(&pkt);
            av_strerror(ret, m_errBuf, sizeof(m_errBuf));
            QLOG_ERROR() << "av_read_frame error: " << m_errBuf;
            break;
        }
        // 读取到音频包
        if(pkt->stream_index == m_audioIndex)
        {
            // 插入音频包队列
            pushPacket(&m_audioPacketQueue, pkt);
        }
        else if(pkt->stream_index == m_videoIndex)
        {
            pushPacket(&m_videoPacketQueue, pkt);
        }
        else
        {
            av_packet_unref(pkt);
        }
    }
    av_packet_free(&pkt);
    if(!m_exit.load())
    {
        while(m_audioFrameQueue.size)
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        exit();
    }
    QLOG_INFO() << "demuxThread exit";
}

void AVTool::Decoder::audioDecode()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    while(true)
    {
        if(m_exit.load()) break;
        // 音频帧队列长度控制
        if(m_audioFrameQueue.size >= m_maxFrameQueueSize)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // 从音频包队列取音频包
        int ret = getPacket(&m_audioPacketQueue, pkt, &m_audioPktDecoder);
        if(ret)
        {
            ret = avcodec_send_packet(m_audioPktDecoder.codecCtx, pkt);
            av_packet_unref(pkt);
            if(ret < 0)
            {
                av_strerror(ret, m_errBuf, sizeof(m_errBuf));
                QLOG_ERROR() << "avcodec_send_packet error: " << m_errBuf;
                continue;
            }
            while(true)
            {
                ret = avcodec_receive_frame(m_audioPktDecoder.codecCtx, frame);
                if(ret == 0)
                {
                    if(m_audSeek)
                    {

                    }
                    // 添加到待播放视频帧队列
                    pushAFrame(frame);
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
    av_packet_free(&pkt);
    av_frame_free(&frame);
    QLOG_INFO() << "audioDecode exit";
}

void AVTool::Decoder::videoDecode()
{
    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    while(true)
    {
        if(m_exit.load()) break;
        if(m_videoFrameQueue.size >= m_maxFrameQueueSize)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        int ret = getPacket(&m_videoPacketQueue, pkt, &m_videoPktDecoder);
        if(ret)
        {
            ret = avcodec_send_packet(m_videoPktDecoder.codecCtx, pkt);
            av_packet_unref(pkt);
            if(ret < 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            {
                av_strerror(ret, m_errBuf, sizeof(m_errBuf));
                QLOG_ERROR() << "avcodec_send_packet error: " << m_errBuf;
                continue;
            }
            while(true)
            {
                ret = avcodec_receive_frame(m_videoPktDecoder.codecCtx, frame);
                if(ret == 0)
                {
                    if(m_vidSeek)
                    {

                    }
                    // 添加到带播放视频帧队列
                    pushVFrame(frame);
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
    av_packet_free(&pkt);
    av_frame_free(&frame);
    QLOG_INFO() << "videoDecode exit";
}

void AVTool::Decoder::pushPacket(FPacketQueue *queue, AVPacket *pkt)
{
    std::lock_guard<std::mutex> lock(queue->mutex);
    av_packet_move_ref(&queue->pktVec[queue->pushIndex].pkt, pkt);
    queue->pktVec[queue->pushIndex].serial = queue->serial;
    queue->pushIndex = (queue->pushIndex + 1) % m_maxPacketQueueSize;
    queue->size++;
}

int AVTool::Decoder::getPacket(FPacketQueue *queue, AVPacket *pkt, FPktDecoder *decoder)
{
    std::unique_lock<std::mutex> lock(queue->mutex);
    while(!queue->size)
    {
        bool ret = queue->cond.wait_for(lock, std::chrono::milliseconds(100), [&](){
            return queue->size > 0;
        });
        if(!ret)
            return 0;
    }

    if(queue->serial != decoder->serial)
    {
        // 序列号不连续的帧证明发生了跳转操作则直接丢弃
        // 并清空解码器缓存
        avcodec_flush_buffers(decoder->codecCtx);
        decoder->serial = queue->pktVec[queue->readIndex].serial;
        return 0;
    }
    av_packet_move_ref(pkt, &queue->pktVec[queue->readIndex].pkt);
    decoder->serial = queue->pktVec[queue->readIndex].serial;
    queue->readIndex = (queue->readIndex + 1) % m_maxPacketQueueSize;
    queue->size--;

    return 1;
}

void AVTool::Decoder::pushAFrame(AVFrame *frame)
{
    std::lock_guard<std::mutex> lock(m_audioFrameQueue.mutex);
    av_frame_move_ref(&m_audioFrameQueue.frameVec[m_audioFrameQueue.pushIndex].frame, frame);
    m_audioFrameQueue.frameVec[m_audioFrameQueue.pushIndex].serial = m_audioPktDecoder.serial;
    m_audioFrameQueue.pushIndex = (m_audioFrameQueue.pushIndex + 1) % m_maxFrameQueueSize;
    m_audioFrameQueue.size++;
}

void AVTool::Decoder::pushVFrame(AVFrame *frame)
{
    std::lock_guard<std::mutex> lock(m_videoFrameQueue.mutex);
    m_videoFrameQueue.frameVec[m_videoFrameQueue.pushIndex].serial = m_videoPktDecoder.serial;
    m_videoFrameQueue.frameVec[m_videoFrameQueue.pushIndex].duration = m_vidFrameRate.den && m_vidFrameRate.num ? av_q2d(AVRational{m_vidFrameRate.den, m_vidFrameRate.num}) : 0.00;
    m_videoFrameQueue.frameVec[m_videoFrameQueue.pushIndex].pts = frame->pts * av_q2d(m_pAvFormatCtx->streams[m_videoIndex]->time_base);
    av_frame_move_ref(&m_videoFrameQueue.frameVec[m_videoFrameQueue.pushIndex].frame, frame);
    m_videoFrameQueue.pushIndex = (m_videoFrameQueue.pushIndex + 1) % m_maxFrameQueueSize;
    m_videoFrameQueue.size++;
}

int AVTool::Decoder::getAFrame(AVFrame *frame)
{
    if(!frame) return 0;
    std::unique_lock<std::mutex> lock(m_audioFrameQueue.mutex);
    while(!m_audioFrameQueue.size) {
        bool ret = m_audioFrameQueue.cond.wait_for(lock,std::chrono::milliseconds(100),
                                                   [&](){return !m_exit && m_audioFrameQueue.size;});
        if(!ret)
            return 0;
    }
    if(m_audioFrameQueue.frameVec[m_audioFrameQueue.readIndex].serial != m_audioPacketQueue.serial) {
        av_frame_unref(&m_audioFrameQueue.frameVec[m_audioFrameQueue.readIndex].frame);
        m_audioFrameQueue.readIndex = (m_audioFrameQueue.readIndex + 1) % m_maxFrameQueueSize;
        m_audioFrameQueue.size--;
        return 0;
    }
    av_frame_move_ref(frame, &m_audioFrameQueue.frameVec[m_audioFrameQueue.readIndex].frame);
    m_audioFrameQueue.readIndex = (m_audioFrameQueue.readIndex + 1) % m_maxFrameQueueSize;
    m_audioFrameQueue.size--;
    return 1;
}

int AVTool::Decoder::getRemainingVFrame()
{
    std::unique_lock<std::mutex> lock(m_videoFrameQueue.mutex);
    if(!m_videoFrameQueue.size)
    {
        return 0;
    }
    return m_videoFrameQueue.size - m_videoFrameQueue.shown;
}

AVTool::Decoder::FFrame *AVTool::Decoder::peekLastVFrame()
{
    FFrame* frame = &m_videoFrameQueue.frameVec[m_videoFrameQueue.readIndex];
    return frame;
}

AVTool::Decoder::FFrame *AVTool::Decoder::peekVFrame()
{
    while(!m_videoFrameQueue.size)
    {
        std::unique_lock<std::mutex> lock(m_videoFrameQueue.mutex);
        bool ret = m_videoFrameQueue.cond.wait_for(lock, std::chrono::milliseconds(100),
                                                   [&](){ return !m_exit.load() && m_videoFrameQueue.size; });
        if(!ret)
            return nullptr;
    }
    int index = (m_videoFrameQueue.readIndex + m_videoFrameQueue.shown) % m_maxFrameQueueSize;
    FFrame* frame = &m_videoFrameQueue.frameVec[index];
    return frame;
}

AVTool::Decoder::FFrame *AVTool::Decoder::peekNextVFrame()
{
    while(m_videoFrameQueue.size < 2)
    {
        std::unique_lock<std::mutex> lock(m_videoFrameQueue.mutex);
        bool ret = m_videoFrameQueue.cond.wait_for(lock, std::chrono::milliseconds(100),
                                                   [&](){ return !m_exit.load() && m_videoFrameQueue.size; });
        if(!ret)
            return nullptr;
    }
    int index = (m_videoFrameQueue.readIndex + m_videoFrameQueue.shown + 1) % m_maxFrameQueueSize;
    FFrame* frame = &m_videoFrameQueue.frameVec[index];
    return frame;
}

void AVTool::Decoder::setNextVFrame()
{
    std::unique_lock<std::mutex> lock(m_videoFrameQueue.mutex);
    if(!m_videoFrameQueue.size)
    {
        return;
    }
    if(!m_videoFrameQueue.shown)
    {
        m_videoFrameQueue.shown = 1;
        return;
    }
    av_frame_unref(&m_videoFrameQueue.frameVec[m_videoFrameQueue.readIndex].frame);
    m_videoFrameQueue.readIndex = (m_videoFrameQueue.readIndex + 1) % m_maxFrameQueueSize;
    m_videoFrameQueue.size--;
}
