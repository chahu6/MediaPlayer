#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class AVPlayer;

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

protected:
    virtual void paintEvent(QPaintEvent *event) override;
    virtual void resizeEvent(QResizeEvent* event) override;
    virtual void keyReleaseEvent(QKeyEvent *event) override;

private:
    void InitUI();

private:
    Ui::Widget *ui;

    AVPlayer* m_player;

    const QString m_vFmt;

    unsigned int m_duration;

    bool m_ptsSliderPressed;

private Q_SLOTS:
    void pauseOnBtnClickSlot();

    void addFile();

    void setVolume(int volume);

    void ptsChangedSlot(unsigned int pts);

    void durationChangedSlot(unsigned int duration);

    void terminateSlot();

    void ptsSliderPressedSlot();

    void ptsSliderMovedSlot();

    void ptsSliderReleaseSlot();

    void seekForwardSlot();

    void seekBackSlot();
};
#endif // WIDGET_H
