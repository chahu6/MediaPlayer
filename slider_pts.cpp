#include "slider_pts.h"

AVPtsSlider::AVPtsSlider(QWidget *parent)
    :QSlider(parent),
    m_percent(0.00)
{
    setMouseTracking(true);
}

AVPtsSlider::~AVPtsSlider()
{}

void AVPtsSlider::setPtsPercent(double percent)
{
    if(percent < 0.0 || percent > 100.0) return;
    m_percent = percent;
    int value = (int)(percent * 100);
    setValue(value);
}
