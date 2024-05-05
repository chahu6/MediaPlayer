#include "AVPlayer.h"

AVPlayer::AVPlayer(QObject *parent)
    : QObject{parent}
{}

bool AVPlayer::play(const QString &url)
{
    clearPlayer();

}

void AVPlayer::clearPlayer()
{

}
