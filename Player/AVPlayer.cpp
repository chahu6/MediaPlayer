#include "AVPlayer.h"
#include <QsLog.h>
extern "C"
{
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libavutil/imgutils.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/time.h>
#include <SDL.h>
}
#include "Decoder.h"
using namespace AVTool;

AVPlayer::AVPlayer(QObject *parent)
    : QObject{parent},
      m_decoder(new Decoder)
{

}

AVPlayer::~AVPlayer()
{

}

bool AVPlayer::play(const QString &url)
{
    clearPlayer();
    if(!m_decoder->decode(url))
    {
        QLOG_INFO() << "decode failed";
        return false;
    }
}

AVPlayer::EPlayerState AVPlayer::playeState()
{
    AVPlayer::EPlayerState state;
    switch (SDL_GetAudioStatus())
    {
        case SDL_AUDIO_PLAYING:
            state=AVPlayer::AV_PLAYING;
            break;
        case SDL_AUDIO_PAUSED:
            state=AVPlayer::AV_PAUSED;
            break;
        case SDL_AUDIO_STOPPED:
            state=AVPlayer::AV_STOPPED;
            break;
        default:
            break;
    }
    return state;
}

void AVPlayer::clearPlayer()
{
    if(playeState() != AV_STOPPED)
    {
        if(playeState() == AV_PLAYING)
        {
            SDL_PauseAudio(1);
        }
        // m_decoder->exit();
        // SDL_CloseAudio();
        // if(m_swsCtx)
        // {
        //     swr_free(&m_swrCtx);
        // }
        // if(m_swsCtx)
        // {
        //     sws_freeContext(m_swxCtx);
        // }
        // m_swrCtx = nullptr;
        // m_swsCtx = nullptr;
    }
}
