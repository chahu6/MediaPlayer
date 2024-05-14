#ifndef OPENGLWIDGET_H
#define OPENGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QTimer>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>

class YUV422Frame;

class OpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit OpenGLWidget(QWidget *parent = nullptr);
    ~OpenGLWidget();

protected:
    virtual void initializeGL() Q_DECL_OVERRIDE;
    virtual void paintGL() Q_DECL_OVERRIDE;
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void mouseDoubleClickEvent(QMouseEvent *event);

signals:
    void mouseClicked();

public slots:
    void onShowYUV(QSharedPointer<YUV422Frame> frame);

private:
    QOpenGLBuffer vbo;
    QOpenGLShaderProgram *program;

    //opengl中y、u、v分量位置
    GLuint posUniformY, posUniformU, posUniformV;

    //纹理
    QOpenGLTexture *textureY = nullptr,*textureU = nullptr,*textureV = nullptr;

    //纹理ID，创建错误返回0
    GLuint m_idY, m_idU, m_idV;

    QTimer m_timer;

    int m_isDoubleClick;

    QSharedPointer<YUV422Frame> m_frame;
};

#endif // OPENGLWIDGET_H
