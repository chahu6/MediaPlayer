#include "AVPlayer.h"
#include <QsLog.h>
#include <QThread>
#include <QFileInfo>
#include "ThreadPool.h"
#include "Media.h"
#include "YUV422Frame.h"
#include "ComMessageBox.h"

AVPlayer::AVPlayer(QObject *parent)
    : QObject{parent},
      m_decoder(new Decoder),
      m_fmtCtx(nullptr),
      m_swrCtx(nullptr),
      m_audioBuf(nullptr),
      m_duration(0),
      m_pause(false),
      m_volume(30),
      m_exit(0),
      m_swsCtx(nullptr),
      m_buffer(nullptr)
{
    m_audioFrame = av_frame_alloc();
}

AVPlayer::~AVPlayer()
{
    if(m_audioFrame)
    {
        av_frame_free(&m_audioFrame);
    }

    clearPlayer();
    if(m_decoder)
    {
        delete m_decoder;
        m_decoder = nullptr;
    }
    if(m_swrCtx)
        swr_free(&m_swrCtx);
    if(m_swsCtx)
        sws_freeContext(m_swsCtx);
    if(m_audioBuf)
        av_free(m_audioBuf);
    if(m_buffer)
        av_free(m_buffer);
}

void AVPlayer::clearPlayer()
{
    if(playeState() != AV_STOPPED)
    {
        m_exit = 1;
        if(playeState() == AV_PLAYING)
        {
            SDL_PauseAudio(1);
        }
        m_decoder->exit();
        SDL_CloseAudio();
        if(m_swrCtx)
        {
            swr_free(&m_swrCtx);
        }
        if(m_swsCtx)
        {
            sws_freeContext(m_swsCtx);
        }
        m_swrCtx = nullptr;
        m_swsCtx = nullptr;
    }
}

bool AVPlayer::play(const QString &url)
{
    QFileInfo fileInfo(url);
    QString suffix = fileInfo.suffix().toLower(); // 获取扩展名并转换为小写
    if(suffix != "mp4" && suffix != "mp3")
    {
        ComMessageBox::error(NULL, QString("目前只支持: mp4、mp3文件类型"));
        return false;
    }

    clearPlayer();
    if(!m_decoder->decode(url))
    {
        QLOG_INFO() << "decode failed";
        return false;
    }

    // 解码成功可获取流时长
    m_duration = m_decoder->duration();
    emit AVDurationChanged(m_duration);

    m_pause = false;
    m_clockInitFlag = -1;

    if(!initSDL())
    {
        QLOG_ERROR() << "init sdl failed!";
        return false;
    }
    initVideo();

    return true;
}

void AVPlayer::pause(bool isPause)
{
    if(SDL_GetAudioStatus() == SDL_AUDIO_STOPPED) return;
    if(isPause)
    {
        if(SDL_GetAudioStatus() == SDL_AUDIO_PLAYING)
        {
            SDL_PauseAudio(1);
            m_pauseTime = av_gettime_relative() / 1000000.0;
            m_pause = true;
        }
    }
    else
    {
        if(SDL_GetAudioStatus() == SDL_AUDIO_PAUSED)
        {
            SDL_PauseAudio(0);
            m_frameTimer += av_gettime_relative() / 1000000.0 - m_pauseTime;
            m_pause = false;
        }
    }
}

void fillAStreamCallback(void* userData, uint8_t* stream, int len)
{
    memset(stream, 0, len);
    AVPlayer* me = (AVPlayer*)userData;
    double audioPts = 0.00;
    while(len > 0)
    {
        if(me->m_exit) break;
        if(me->m_audioBufIndex >= me->m_audioBufSize)
        {
            int ret = me->m_decoder->getAFrame(me->m_audioFrame);
            if(ret)
            {
                me->m_audioBufIndex = 0;
                if((me->m_targetSampleFmt != me->m_audioFrame->format ||
                     me->m_targetChannelLayout != me->m_audioFrame->channel_layout ||
                     me->m_targetFreq != me->m_audioFrame->sample_rate||
                     me->m_targetNbSamples!=me->m_audioFrame->nb_samples) &&
                    !me->m_swrCtx)
                {
                    me->m_swrCtx = swr_alloc_set_opts(nullptr, me->m_targetChannelLayout, me->m_targetSampleFmt, me->m_targetFreq,
                          me->m_audioFrame->channel_layout, (enum AVSampleFormat)me->m_audioFrame->format, me->m_audioFrame->sample_rate,
                          0, nullptr);
                    if(!me->m_swrCtx || swr_init(me->m_swrCtx) < 0)
                    {
                        QLOG_ERROR() << "swr_init failed";
                        return;
                    }
                }
                if(me->m_swrCtx)
                {
                    const uint8_t** in = (const uint8_t**)me->m_audioFrame->extended_data;
                    int out_count = (uint64_t)me->m_audioFrame->nb_samples * me->m_targetFreq / me->m_audioFrame->sample_rate + 256;
                    int out_size = av_samples_get_buffer_size(nullptr, me->m_targetChannelLayout, out_count, me->m_targetSampleFmt, 0);
                    if(out_size < 0)
                    {
                        QLOG_ERROR() << "av_samples_get_buffer_size failed";
                        return;
                    }
                    av_fast_malloc(&me->m_audioBuf, &me->m_audioBufSize, out_size);
                    if(!me->m_audioBuf)
                    {
                        QLOG_ERROR() << "av_fast_malloc failed";
                        return;
                    }
                    // 进行重采样转换处理,返回转换后的样本数量
                    int len2 = swr_convert(me->m_swrCtx, &me->m_audioBuf, out_count, in, me->m_audioFrame->nb_samples);
                    if(len2 < 0)
                    {
                        QLOG_ERROR() << "swr_convert failed";
                        return;
                    }
                    me->m_audioBufSize = av_samples_get_buffer_size(nullptr, me->m_targetChannels, len2, me->m_targetSampleFmt, 0);
                }
                else
                {
                    me->m_audioBufSize = av_samples_get_buffer_size(nullptr, me->m_targetChannels, me->m_audioFrame->nb_samples, me->m_targetSampleFmt, 0);
                    av_fast_malloc(&me->m_audioBuf, &me->m_audioBufSize, me->m_audioBufSize + 256);
                    if(!me->m_audioBuf)
                    {
                        QLOG_ERROR() << "av_fast_malloc failed";
                        return;
                    }
                    memcpy(me->m_audioBuf, me->m_audioFrame->data[0], me->m_audioBufSize);
                }
                audioPts = me->m_audioFrame->pts * av_q2d(me->m_fmtCtx->streams[me->m_audioIndex]->time_base);
                av_frame_unref(me->m_audioFrame);
            }
            else
            {
                // 判断是否真正播放到文件末尾
                if(me->m_decoder->isExit())
                {
                    emit me->AVTerminate();
                }
                return;
            }
        }
        int len1 = me->m_audioBufSize - me->m_audioBufIndex;
        len1 = (len1 > len ? len : len1);
        SDL_MixAudio(stream, me->m_audioBuf + me->m_audioBufIndex, len1, me->m_volume);
        len -= len1;
        me->m_audioBufIndex += len1;
        stream += len1;
    }
    // 记录音频时钟
    me->m_audioClock.setClock(audioPts);
    //发送时间戳变化信号,因为进度以整数秒单位变化展示，
    //所以大于一秒才发送，避免过于频繁的信号槽通信消耗性能
    uint32_t _pts = (uint32_t)audioPts;
    if(me->m_lastAudPts != _pts)
    {
        emit me->AVPtsChanged(_pts);
        me->m_lastAudPts = _pts;
    }
}

AVPlayer::EPlayerState AVPlayer::playeState()
{
    AVPlayer::EPlayerState state;
    switch (SDL_GetAudioStatus())
    {
        case SDL_AUDIO_PLAYING:
            state = AVPlayer::AV_PLAYING;
            break;
        case SDL_AUDIO_PAUSED:
            state = AVPlayer::AV_PAUSED;
            break;
        case SDL_AUDIO_STOPPED:
            state = AVPlayer::AV_STOPPED;
            break;
        default:
            break;
    }

    return state;
}

int AVPlayer::initSDL()
{
    if(SDL_Init(SDL_INIT_AUDIO) != 0)
    {
        QLOG_ERROR() << "SDL_Init failed";
        return 0;
    }

    m_exit = 0;

    m_audioBufSize = 0;
    m_audioBufIndex = 0;

    m_lastAudPts = -1;

    m_audioCodecPar = m_decoder->audioCodecPar();

    SDL_AudioSpec wanted_spec;
    wanted_spec.channels = m_audioCodecPar->channels;
    wanted_spec.freq = m_audioCodecPar->sample_rate;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.silence = 0;
    wanted_spec.callback = fillAStreamCallback;
    wanted_spec.userdata = this;
    wanted_spec.samples = m_audioCodecPar->frame_size;

    if(SDL_OpenAudio(&wanted_spec, nullptr) < 0)
    {
        QLOG_ERROR() << "SDL_OpenAudio failed";
        return 0;
    }
    m_targetSampleFmt = AV_SAMPLE_FMT_S16;
    m_targetChannels = m_audioCodecPar->channels;
    m_targetFreq = m_audioCodecPar->sample_rate;
    m_targetChannelLayout = av_get_default_channel_layout(m_targetChannels);
    m_targetNbSamples = m_audioCodecPar->frame_size;
    m_audioIndex = m_decoder->audioIndex();
    m_fmtCtx = m_decoder->formatContext();

    SDL_PauseAudio(0);
    return 1;
}

void AVPlayer::initVideo()
{
    m_frameTimer = 0.00;

    m_videoCodecPar=m_decoder->videoCodecPar();
    m_videoIndex=m_decoder->videoIndex();

    m_imageWidth = m_videoCodecPar->width;
    m_imageHeight = m_videoCodecPar->height;
    m_aspectRatio = m_imageHeight != 0 && m_imageWidth != 0 ? static_cast<float>(m_imageWidth) / static_cast<float>(m_imageHeight) : 1.f;

    m_dstPixFmt = AV_PIX_FMT_YUV422P;
    m_swsFlags = SWS_BICUBIC;

    // 创建输出视频帧对象以及分配相应的缓冲区
    // 分配存储转换后帧数据的buffer内存
    int bufSize = av_image_get_buffer_size(m_dstPixFmt, m_imageWidth, m_imageHeight, 1);
    m_buffer = (uint8_t*)av_realloc(m_buffer, bufSize * sizeof(uint8_t));
    av_image_fill_arrays(m_pixels, m_pitch, m_buffer, m_dstPixFmt, m_imageWidth, m_imageHeight, 1);

    // 视频帧播放回调递插入线程池任务队列
    ThreadPool::instance().commit([this](){
        this->videoCallback();
    });
}

void AVPlayer::videoCallback()
{
    double time = 0.00;
    double duration = 0.00;
    double delay = 0.00;
    if(m_clockInitFlag == -1)
    {
        initAVClock();
    }

    do
    {
        if(m_exit) break;

        if(m_pause)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        if(m_decoder->getRemainingVFrame())
        {
            FFrame* lastFrame = m_decoder->peekLastVFrame();
            FFrame* frame = m_decoder->peekVFrame();

            if(frame->serial != m_decoder->vidPktSerial())
            {
                m_decoder->setNextVFrame();
                continue;
            }

            if(frame->serial != lastFrame->serial)
            {
                m_frameTimer = av_gettime_relative() / 1000000.0;
            }

            duration = vpDuration(lastFrame, frame);
            delay = computeTargetDelay(duration);

            time = av_gettime_relative() / 1000000.0; // 被除以 1000000.0 来将单位从微秒（microseconds）转换为秒（seconds）。

            // 显示时长未到
            if(time < m_frameTimer + delay)
            {
                QThread::msleep((uint32_t)(FFMIN(AV_SYNC_REJUDGESHOLD, m_frameTimer + delay - time) * 1000));
                continue;
            }

            m_frameTimer += delay;
            if(time - m_frameTimer > AV_SYNC_THRESHOLD_MAX)
            {
                m_frameTimer = time;
            }

            // 队列中未显示帧一帧以上执行逻辑丢帧判断
            if(m_decoder->getRemainingVFrame() > 1)
            {
                FFrame* nextFrame = m_decoder->peekNextVFrame();
                duration = nextFrame->pts - frame->pts;
                // 若主时钟超前到大于当前帧理论显示应持续的时间了，则当前帧立即丢弃
                if(time > m_frameTimer + duration)
                {
                    m_decoder->setNextVFrame();
                    QLOG_INFO() << "abandon vFrame";
                    continue;
                }
            }

            displayImage(&frame->frame);
            // 读索引后移
            m_decoder->setNextVFrame();
        }
        else
        {
            QThread::msleep(10);
        }
    }while(true);
}

void AVPlayer::displayImage(AVFrame *frame)
{
    if(frame == nullptr) return;
    // 判断是否需要格式转换
    if((m_videoCodecPar->width != m_imageWidth ||
        m_videoCodecPar->height != m_imageHeight ||
        m_videoCodecPar->format != m_dstPixFmt) && !m_swsCtx)
    {
        // 创建格式转换器, 指定缩放算法,转换过程中不增加任何滤镜特效处理
        m_swsCtx = sws_getCachedContext(m_swsCtx, frame->width, frame->height,
            (enum AVPixelFormat)frame->format, m_imageWidth, m_imageHeight, m_dstPixFmt,
                                        m_swsFlags, nullptr, nullptr, nullptr);
    }
    if(m_swsCtx)
    {
        // 进行格式转换处理
        sws_scale(m_swsCtx, static_cast<const uint8_t* const*>(frame->data), frame->linesize, 0, frame->height, m_pixels, m_pitch);
        emit frameChanged(QSharedPointer<YUV422Frame>::create(m_pixels[0], m_imageWidth, m_imageHeight));
    }
    else
    {
        // emit frameChanged(QSharedPointer<YUV422Frame>::create((uint8_t*)frame->data[0], m_imageWidth, m_imageHeight));
        QLOG_ERROR() << "fail to sws_getContext()";
    }

    //记录视频时钟
    m_videoClock.setClock(frame->pts * av_q2d(m_fmtCtx->streams[m_videoIndex]->time_base));
}

void AVPlayer::initAVClock()
{
    m_audioClock.setClock(0.00);
    m_videoClock.setClock(0.00);
    m_clockInitFlag = 1;
}

double AVPlayer::computeTargetDelay(double delay)
{
    // 视频当前显示帧与当前播放音频帧时间戳插值
    double diff = m_videoClock.getClock() - m_audioClock.getClock();

    // 计算同步阈值
    double sync = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));

    //不同步时间超过阈值直接放弃同步
    if(!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD)
    {
        if(diff <= -sync)
        {
            delay = FFMAX(0, diff + delay);
        }
        else if(diff >= sync && delay > AV_SYNC_FRAMEDUP_THRESHOLD)
        {
            delay = diff + delay;
        }
        else if(diff >= sync)
        {
            delay = 2 * delay;
        }
    }
    return delay;
}

double AVPlayer::vpDuration(FFrame *lastFrame, FFrame *currentFrame)
{
    if(currentFrame->serial == lastFrame->serial)
    {
        double duration = currentFrame->pts - lastFrame->pts;
        if(std::isnan(duration) || duration > AV_NOSYNC_THRESHOLD)
        {
            return lastFrame->duration;
        }
        else
        {
            return duration;
        }
    }
    else
    {
        return 0.00;
    }
}
