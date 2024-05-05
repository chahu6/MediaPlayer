#ifndef AVPTSSLIDER_H
#define AVPTSSLIDER_H

#include <QSlider>

class AVPtsSlider : public QSlider
{
    Q_OBJECT
public:
    AVPtsSlider(QWidget* parent = Q_NULLPTR);
    ~AVPtsSlider();
};

#endif // AVPTSSLIDER_H
