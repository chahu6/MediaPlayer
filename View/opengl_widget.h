#ifndef OPENGLWIDGET_H
#define OPENGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QTimer>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <Eigen/Dense>

class YUV422Frame;

class OpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit OpenGLWidget(QWidget *parent = nullptr);
    ~OpenGLWidget();

protected:
    virtual void initializeGL() Q_DECL_OVERRIDE;
    virtual void resizeGL(int w, int h) Q_DECL_OVERRIDE;
    virtual void paintGL() Q_DECL_OVERRIDE;
    virtual void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    virtual void mouseDoubleClickEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

signals:
    void mouseClicked();
    void mouseDoubleClicked();

public slots:
    void onShowYUV(QSharedPointer<YUV422Frame> frame);

private:
    QOpenGLBuffer vbo;
    QOpenGLShaderProgram *program;

    //opengl中y、u、v分量位置
    GLuint posUniformY, posUniformU, posUniformV;
    GLuint posUniformTransform;

    //纹理
    QOpenGLTexture *textureY = nullptr,*textureU = nullptr,*textureV = nullptr;

    //纹理ID，创建错误返回0
    GLuint m_idY, m_idU, m_idV;

    QTimer m_timer;

    int m_isDoubleClick;

    QSharedPointer<YUV422Frame> m_frame;

    int m_dest_w, m_dest_h;
    float m_dest_aspect_ratio = 1.f; // 显示窗口的宽高比
    float m_src_aspect_ratio = 1.f; // 原始视频宽高比

    Eigen::Matrix4f m_transform;

public:
    inline void setSrcAspectRatio(float newSrcAspectRatio) { m_src_aspect_ratio = newSrcAspectRatio; }
};

#endif // OPENGLWIDGET_H
