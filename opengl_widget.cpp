#include "opengl_widget.h"
#include <QsLog.h>
#include "YUV422Frame.h"

#define VERTEXIN 0
#define TEXTUREIN 1

const char* vshadersrc = R"(
    #version 450 core
    layout(location = 0) in vec2 vertexIn;
    layout(location = 1) in vec2 textureIn;

    layout(location = 0) out vec2 textureOut;

    uniform mat4 transform;

    void main(void)
    {
        gl_Position = transform * vec4(vertexIn, 0.0, 1.0);
        textureOut = textureIn;
    }
)";

const char* fshadersrc = R"(
    #version 450 core
    layout(location = 0) out vec4 o_Color;

    layout(location = 0) in vec2 textureOut;

    uniform sampler2D tex_y;
    uniform sampler2D tex_u;
    uniform sampler2D tex_v;

    void main(void)
    {
        vec3 yuv;
        vec3 rgb;
        yuv.x = texture2D(tex_y, textureOut).r;
        yuv.y = texture2D(tex_u, textureOut).r - 0.5;
        yuv.z = texture2D(tex_v, textureOut).r - 0.5;
        rgb = mat3( 1,        1,        1,
                    0,       -0.39465, 2.03211,
                    1.13983, -0.58060, 0) * yuv;
        o_Color = vec4(rgb, 1);
    }
)";

OpenGLWidget::OpenGLWidget(QWidget *parent)
    :QOpenGLWidget(parent),
    m_isDoubleClick(0),
    m_transform(Eigen::Matrix4f::Identity())
{
    connect(&m_timer, &QTimer::timeout, [this]()
    {
        if(!this->m_isDoubleClick)
        {
            emit this->mouseClicked();
        }
        this->m_isDoubleClick = 0;
        this->m_timer.stop();
    });
    m_timer.setInterval(400);
}

OpenGLWidget::~OpenGLWidget()
{
    makeCurrent();
    vbo.destroy();
    textureY->destroy();
    textureU->destroy();
    textureV->destroy();
    doneCurrent();
}

void OpenGLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.180, 0.180, 0.212, 0.0);
    glEnable(GL_DEPTH_TEST);

    static const GLfloat vertices[]{
        // 顶点坐标
        -1.0f, -1.0f,
        -1.0f, +1.0f,
        +1.0f, +1.0f,
        +1.0f, -1.0f,
        //纹理坐标
        0.0f,1.0f,
        0.0f,0.0f,
        1.0f,0.0f,
        1.0f,1.0f,
    };

    vbo.create();
    vbo.bind();
    vbo.allocate(vertices, sizeof(vertices));

    program = new QOpenGLShaderProgram(this);
    // 编译着色器
    program->addShaderFromSourceCode(QOpenGLShader::Vertex, vshadersrc);
    program->addShaderFromSourceCode(QOpenGLShader::Fragment, fshadersrc);

    // 绑定输入的顶点坐标和纹理坐标属性
    program->bindAttributeLocation("vertexIn", VERTEXIN);
    program->bindAttributeLocation("textureIn", TEXTUREIN);

    // 链接
    if(!program->link())
    {
        QLOG_ERROR() << "programe link error";
        close();
    }

    // 绑定着色器管道以供使用
    if(!program->bind())
    {
        QLOG_ERROR() << "program bind error";
        close();
    }

    // 启动并且设置顶点位置和纹理坐标
    program->enableAttributeArray(VERTEXIN);
    program->enableAttributeArray(TEXTUREIN);
    program->setAttributeBuffer(VERTEXIN, GL_FLOAT, 0, 2, 2 * sizeof(GLfloat));
    program->setAttributeBuffer(TEXTUREIN, GL_FLOAT, 8 * sizeof(GLfloat), 2, 2 * sizeof(GLfloat));

    // 定位shader中的uniform变量
    posUniformY = program->uniformLocation("tex_y");
    posUniformU = program->uniformLocation("tex_u");
    posUniformV = program->uniformLocation("tex_v");
    posUniformTransform = program->uniformLocation("transform");

    // 创建纹理并且获取ID
    textureY = new QOpenGLTexture(QOpenGLTexture::Target2D);
    textureU = new QOpenGLTexture(QOpenGLTexture::Target2D);
    textureV = new QOpenGLTexture(QOpenGLTexture::Target2D);
    textureY->create();
    textureU->create();
    textureV->create();
    m_idY = textureY->textureId();
    m_idU = textureU->textureId();
    m_idV = textureV->textureId();
}

void OpenGLWidget::resizeGL(int w, int h)
{
    if(h == 0) return;

    m_dest_w = w;
    m_dest_h = h;
    m_dest_aspect_ratio = static_cast<float>(w) / static_cast<float>(h);
}

void OpenGLWidget::paintGL()
{
    ///------------------------
    /// glTexImage2D(
    /// GLenum target,
    /// GLint level,
    /// GLint internalformat,
    /// GLsizei width,
    /// GLsizei height,
    /// GLint border,
    /// GLenum format,
    /// GLenum type,
    /// const GLvoid* pixels)
    /* 函数很长，参数也不少，所以我们一个一个地讲解：
    第一个参数指定了纹理目标(Target)。设置为GL_TEXTURE_2D意味着会生成与当前绑定的纹理对象在同一个目标上的纹理
            （任何绑定到GL_TEXTURE_1D和GL_TEXTURE_3D的纹理不会受到影响）。
    第二个参数为纹理指定多级渐远纹理的级别，如果你希望单独手动设置每个多级渐远纹理的级别的话。这里我们填0，也就是基本级别。
    第三个参数告诉OpenGL我们希望把纹理储存为何种格式。
    第四个和第五个参数设置最终的纹理的宽度和高度。我们之前加载图像的时候储存了它们，所以我们使用对应的变量。
    第六个参数应该总是被设为0（历史遗留的问题）。
    第七个参数定义了源图的格式。
    第八个参数定义了源图的数据类型。
    最后一个参数是真正的图像数据。

    当调用glTexImage2D时，当前绑定的纹理对象就会被附加上纹理图像。然而，目前只有基本级别(Base-level)的纹理图像被加载了，
    如果要使用多级渐远纹理，我们必须手动设置所有不同的图像（不断递增第二个参数）。
    或者，直接在生成纹理之后调用glGenerateMipmap。这会为当前绑定的纹理自动生成所有需要的多级渐远纹理。
    */

    if(m_frame.isNull()) return;

    // 画面保持比例缩放
    uint32_t videoW = m_frame->getPixelW();
    uint32_t videoH = m_frame->getPixelH();

    m_src_aspect_ratio = (float)videoW / (float)videoH;

    if(m_dest_aspect_ratio >= m_src_aspect_ratio)
    {
        float w = m_src_aspect_ratio * m_dest_h;
        m_transform(0, 0) = w / m_dest_w;
        m_transform(1, 1) = 1.f;
    }
    else
    {
        float h = m_dest_w / m_src_aspect_ratio;
        m_transform(0, 0) = 1.f;
        m_transform(1, 1) = h / m_dest_h;
    }

    // 激活纹理单元GL_TEXTURE0, 系统里面的
    glActiveTexture(GL_TEXTURE0);
    // 绑定y分量纹理对象ID到激活的纹理单元
    glBindTexture(GL_TEXTURE_2D, m_idY);
    //使用内存中的数据创建真正的y分量纹理数据
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, videoW, videoH, 0, GL_RED, GL_UNSIGNED_BYTE, m_frame->getBufY());//m_yPtr 大小是(videoW*videoH)
    //https://blog.csdn.net/xipiaoyouzi/article/details/53584798 纹理参数解析
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    //激活纹理单元GL_TEXTURE1
    glActiveTexture(GL_TEXTURE1);
    //绑定u分量纹理对象id到激活的纹理单元
    glBindTexture(GL_TEXTURE_2D,m_idU);
    //使用内存中的数据创建真正的u分量纹理数据
    glTexImage2D(GL_TEXTURE_2D,0,GL_RED,videoW>>1 , videoH,0,GL_RED,GL_UNSIGNED_BYTE,m_frame->getBufU());//m_uPtr 大小是(videoW*videoH / 2)
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    //激活纹理单元GL_TEXTURE2
    glActiveTexture(GL_TEXTURE2);
    //绑定v分量纹理对象id到激活的纹理单元
    glBindTexture(GL_TEXTURE_2D,m_idV);
    //使用内存中的数据创建真正的v分量纹理数据
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, videoW>>1 , videoH, 0, GL_RED, GL_UNSIGNED_BYTE, m_frame->getBufV());//m_vPtr 大小是(videoW*videoH / 2)
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // 计算比例缩放矩阵
    glUniformMatrix4fv(posUniformTransform, 1, GL_FALSE, m_transform.data());

    //指定y纹理要使用新值
    glUniform1i(posUniformY, 0);
    //指定u纹理要使用新值
    glUniform1i(posUniformU, 1);
    //指定v纹理要使用新值
    glUniform1i(posUniformV, 2);
    //使用顶点数组方式绘制图形
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void OpenGLWidget::onShowYUV(QSharedPointer<YUV422Frame> frame)
{
    if(frame.isNull())
    {
        QLOG_ERROR() << "frame is nullptr!";
    }
    if(!m_frame.isNull())
    {
        m_frame.reset();
    }
    m_frame = frame;
    update();
}

void OpenGLWidget::mouseReleaseEvent(QMouseEvent *event)
{

}

void OpenGLWidget::mouseDoubleClickEvent(QMouseEvent *event)
{

}
