#include "widget.h"
#include "ui_widget.h"
#include <QDir>
#include <QFileDialog>
#include <QPainter>
#include "AVPlayer.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
    , m_vFmt("视频文件(*.mp4 *.mov *.avi *.mkv *.wmv *.flv *.webm *.mpeg *.mpg *.3gp *.m4v *.rmvb *.vob *.ts *.mts *.m2ts *.f4v *.divx *.xvid)")
{
    ui->setupUi(this);
    QString title = QString("%1 V%2 (64-bit, windows)").arg(QCoreApplication::applicationName()).arg(QCoreApplication::applicationVersion());
    setWindowTitle(title);
    InitUI();

    m_player = new AVPlayer(this);

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
            }
        }
    });
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
}

void Widget::InitUI()
{
    ui->label_seperator->setAlignment(Qt::AlignCenter);
    ui->label_pts->setAlignment(Qt::AlignCenter);
    ui->label_duration->setAlignment(Qt::AlignCenter);
    ui->label_volume->setAlignment(Qt::AlignCenter);
    ui->lineEdit_input->setText("D:/QTProject/MediaPlayer/02test.mp4");

    ui->slider_AVPts->setEnabled(false);
    ui->btn_forward->setEnabled(false);
    ui->btn_back->setEnabled(false);
    ui->btn_pauseon->setEnabled(false);
}

void Widget::pauseOnBtnClickSlot()
{

}

void Widget::addFile()
{
    QString url = QFileDialog::getOpenFileName(this, "选择文件", QDir::currentPath(), m_vFmt);
    ui->lineEdit_input->setText(url);
}

void Widget::setVolume(int volume)
{

}

void Widget::ptsChangedSlot(unsigned int duration)
{

}

void Widget::durationChangedSlot(unsigned int pts)
{

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
