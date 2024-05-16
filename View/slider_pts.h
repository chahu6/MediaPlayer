#ifndef AVPTSSLIDER_H
#define AVPTSSLIDER_H

#include <QSlider>

class AVPtsSlider : public QSlider
{
    Q_OBJECT
public:
    AVPtsSlider(QWidget* parent = Q_NULLPTR);
    ~AVPtsSlider();

    void setPtsPercent(double percent);
    double ptsPercent();

protected:
    virtual bool event(QEvent* e) Q_DECL_OVERRIDE;
    virtual void mousePressEvent(QMouseEvent* e) Q_DECL_OVERRIDE;
    virtual void mouseMoveEvent(QMouseEvent* e) Q_DECL_OVERRIDE;
    virtual void mouseReleaseEvent(QMouseEvent* e) Q_DECL_OVERRIDE;
    virtual void paintEvent(QPaintEvent* e) Q_DECL_OVERRIDE;

private:
    bool m_isEnter;
    double m_percent;
    double m_cursorXPer;

public:
    inline double cursorXPercent() const { return m_cursorXPer; }
};

#endif // AVPTSSLIDER_H
