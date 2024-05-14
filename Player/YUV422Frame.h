#ifndef YUV422FRAME_H
#define YUV422FRAME_H

#include <memory>

class YUV422Frame
{
public:
    YUV422Frame(uint8_t* buffer, uint32_t pixelW, uint32_t pixelH)
        :m_buffer(nullptr)
    {
        create(buffer, pixelW, pixelH);
    }

    ~YUV422Frame()
    {
        if(m_buffer != nullptr)
        {
            free(m_buffer);
        }
    }
private:
    void create(uint8_t* buffer, uint32_t pixelW, uint32_t pixelH)
    {
        m_pixelW = pixelW;
        m_pixelH = pixelH;
        int sizeY = m_pixelW * m_pixelH;
        if(!m_buffer)
        {
            m_buffer = (uint8_t*)malloc(sizeY * 2);
            memcpy(m_buffer, buffer, sizeY);
            memcpy(m_buffer + sizeY, buffer + sizeY, sizeY >> 1);
            memcpy(m_buffer + sizeY * 3 / 2, buffer + sizeY * 3 / 2, sizeY >> 1);
        }
    }

private:
    uint32_t m_pixelW;

    uint32_t m_pixelH;

    uint8_t* m_buffer;

public:
    inline uint8_t* getBufY() const { return m_buffer; }

    inline uint8_t* getBufV() const { return m_buffer+m_pixelW*m_pixelH*3/2; }

    inline uint8_t* getBufU() const { return m_buffer+m_pixelW*m_pixelH; }

    inline uint32_t getPixelW() const { return m_pixelW; }

    inline uint32_t getPixelH() const { return m_pixelH; }
};

#endif // YUV420FRAME_H
