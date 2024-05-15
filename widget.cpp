#include "widget.h"
#include "ui_widget.h"
#include <QDir>
#include <QFileDialog>
#include <QPainter>
#include "AVPlayer.h"
#include "YUV422Frame.h"

Q_DECLARE_METATYPE(QSharedPointer<YUV422Frame>)

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
    , m_vFmt("视频文件(*.mp4 *.mov *.avi *.mkv *.wmv *.flv *.webm *.mpeg *.mpg *.3gp *.m4v *.rmvb *.vob *.ts *.mts *.m2ts *.f4v *.divx *.xvid)")
    , m_duration(0)
    , m_ptsSliderPressed(false)
{
    ui->setupUi(this);
    QString title = QString("%1 V%2 (64-bit, windows)").arg(QCoreApplication::applicationName()).arg(QCoreApplication::applicationVersion());
    setWindowTitle(title);

    m_player = new AVPlayer(this);

    InitUI();

    /**
     * 主要特点是它确保信号和槽之间的通信是异步的，通过 Qt 的事件循环进行。具体来说，当发射一个信号时，如果信号与槽的连接是 Qt::QueuedConnection 类型的，
     * 那么槽函数的调用不会立即执行，而是会被放入接收对象（即槽函数所属的对象）所在线程的事件队列中，等待事件循环下一次迭代时执行。
    */
    connect(m_player, &AVPlayer::frameChanged, ui->opengl_widget, &OpenGLWidget::onShowYUV, Qt::QueuedConnection);

    connect(ui->btn_pauseon, &QPushButton::clicked, this, &Widget::pauseOnBtnClickSlot);

    connect(ui->btn_addFile,&QPushButton::clicked,this,&Widget::addFile);

    connect(ui->btn_play, &QPushButton::clicked, this, [this](){
        const QString url = ui->lineEdit_input->text();
        if(url.count())
        {
            if(m_player->play(url))
            {
                ui->slider_AVPts->setEnabled(true);
                ui->btn_forward->setEnabled(true);
                ui->btn_back->setEnabled(true);
                ui->btn_pauseon->setEnabled(true);
                ui->btn_play->setEnabled(false);
            }
        }
    });

    connect(m_player, &AVPlayer::AVDurationChanged, this, &Widget::durationChangedSlot);

    connect(m_player, &AVPlayer::AVPtsChanged, this, &Widget::ptsChangedSlot);

    connect(ui->slider_volume, &QSlider::valueChanged, this, &Widget::setVolume);
}

Widget::~Widget()
{
    delete ui;
}

void Widget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    // 背景换个颜色
    QPainter painter(this);
    painter.setBrush(QBrush(QColor(46,46,54)));
    painter.drawRect(rect());
}

void Widget::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
}

void Widget::keyReleaseEvent(QKeyEvent *event)
{
    (void)event;
}

void Widget::InitUI()
{
    ui->label_seperator->setAlignment(Qt::AlignCenter);
    ui->label_pts->setAlignment(Qt::AlignCenter);
    ui->label_duration->setAlignment(Qt::AlignCenter);
    ui->label_volume->setAlignment(Qt::AlignCenter);
    ui->lineEdit_input->setText("D:/QTProject/MediaPlayer/02test.mp4");

    ui->slider_AVPts->setEnabled(false);
    ui->slider_AVPts->setMaximum(100);
    ui->btn_forward->setEnabled(false);
    ui->btn_back->setEnabled(false);
    ui->btn_pauseon->setEnabled(false);

    ui->slider_volume->setMaximum(100);
    ui->slider_volume->setValue(m_player->getVolume());
    ui->label_volume->setText(QString("%1%").arg(m_player->getVolume()));
}

void Widget::pauseOnBtnClickSlot()
{
    switch (m_player->playeState())
    {
        case AVPlayer::AV_PLAYING:
            m_player->pause(true);
            ui->btn_pauseon->setText(QString("继续"));
            break;
        case AVPlayer::AV_PAUSED:
            m_player->pause(false);
            ui->btn_pauseon->setText(QString("暂停"));
            break;
        default:
            break;
    }
}

void Widget::addFile()
{
    QString url = QFileDialog::getOpenFileName(this, "选择文件", QDir::currentPath(), m_vFmt);
    ui->lineEdit_input->setText(url);
}

void Widget::setVolume(int volume)
{
    m_player->setVolume(volume);
    ui->label_volume->setText(QString("%1%").arg(volume));
}

void Widget::ptsChangedSlot(unsigned int pts)
{
    if(m_ptsSliderPressed) return;

    ui->slider_AVPts->setPtsPercent((double)pts / m_duration);
    ui->label_pts->setText(QString("%1:%2").arg(pts / 60, 2, 10, QLatin1Char('0'))
                               .arg(pts % 60, 2, 10, QLatin1Char('0')));
}

void Widget::durationChangedSlot(unsigned int duration)
{
    ui->label_duration->setText(QString("%1:%2").arg(duration / 60, 2, 10, QLatin1Char('0')).arg(duration % 60, 2, 10, QLatin1Char('0')));
    m_duration = duration;
}

void Widget::terminateSlot()
{

}

void Widget::ptsSliderPressedSlot()
{

}

void Widget::ptsSliderMovedSlot()
{

}

void Widget::ptsSliderReleaseSlot()
{

}

void Widget::seekForwardSlot()
{

}

void Widget::seekBackSlot()
{

}
