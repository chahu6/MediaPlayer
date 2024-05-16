#include "sound_slider.h"

#include <QMouseEvent>

SoundSlider::SoundSlider(QWidget *parent)
    :QSlider(parent)
{}

bool SoundSlider::event(QEvent *e)
{
    if(e->type() == QEvent::Resize)
    {
        setMaximum(this->width() - 1);
    }
    return QSlider::event(e);
}

void SoundSlider::mousePressEvent(QMouseEvent *e)
{
    double posX = e->pos().x() < 0 ? 0 : e->pos().x() > this->width() - 1 ? this->width() - 1 : e->pos().x();
    setValue(posX);

    emit volumeChanged(posX * 100 / (this->width() - 1));
}

void SoundSlider::mouseMoveEvent(QMouseEvent *e)
{
    double posX = e->pos().x() < 0 ? 0 : e->pos().x() > this->width() ? this->width() : e->pos().x();
    setValue(posX);

    emit volumeChanged(posX * 100 / (this->width() - 1));
}
