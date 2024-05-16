#ifndef SOUNDSLIDER_H
#define SOUNDSLIDER_H

#include <QSlider>

class SoundSlider : public QSlider
{
    Q_OBJECT
public:
    SoundSlider(QWidget* parent = Q_NULLPTR);
    ~SoundSlider() = default;

protected:
    virtual bool event(QEvent* e) Q_DECL_OVERRIDE;
    virtual void mousePressEvent(QMouseEvent* e) Q_DECL_OVERRIDE;
    virtual void mouseMoveEvent(QMouseEvent* e) Q_DECL_OVERRIDE;

signals:
    void volumeChanged(int volume);
};

#endif // SOUNDSLIDER_H
