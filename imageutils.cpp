﻿#include "imageutils.h"

FImage::FImage(){}

FImage::FImage(const QString &fileName, const char *format):
    _qimage{fileName, format}
{
    this->inItMat();
}

FImage::FImage(const QImage &image):
    _qimage{std::forward<const QImage&>(image)}
{
    this->inItMat();
}

FImage::FImage(QImage &&other) noexcept:
    _qimage{std::forward<QImage&&>(other)}
{
    this->inItMat();
}

FImage::FImage(const FImage &fimage):
    FImage(fimage._qimage)
{
}

FImage::FImage(FImage &&fimage) noexcept:
    FImage(std::forward<QImage&&>(fimage._qimage))
{
}

FImage &FImage::operator=(const FImage &fimage)
{
    _qimage = fimage._qimage;
    return *this;
}

FImage &FImage::operator=(FImage &&fimage) noexcept
{
    _qimage = std::forward<QImage&&>(fimage._qimage);
    return *this;
}

FImage::operator QImage()
{
    return _qimage;
}

cv::Mat FImage::mat() const
{
    return this->_mat.clone();
}

FImage& FImage::GaussianBlur(int radius)
{
    if(radius > 0)
    {
        int ksize = radius * 2 + 1;
        cv::GaussianBlur(_mat, _mat, cv::Size(ksize,ksize), 0, 0);
    }
    if(radius < 0)
        qWarning()<<"The blur radius must be greater than 0";

    return *this;
}

QPixmap FImage::toQPixmap()
{
    return QPixmap::fromImage(_qimage);
}

QImage& FImage::qImage()
{
    return _qimage;
}

void FImage::inItMat()
{
    switch(this->_qimage.format())
    {
    case QImage::Format_Grayscale8: // 8-bit, 1 channel 灰度图
        _mat = cv::Mat(_qimage.height(),_qimage.width(),CV_8UC1, (void*)(_qimage.constBits()),_qimage.bytesPerLine());
        break;
    case QImage::Format_ARGB32: // uint32存储0xAARRGGBB，pc一般小端存储低位在前，实际字节顺序就成了BGRA
        [[fallthrough]];
    case QImage::Format_RGB32: // Alpha为FF
        [[fallthrough]];
    case QImage::Format_ARGB32_Premultiplied:
        _mat = cv::Mat(_qimage.height(),_qimage.width(),CV_8UC4, (void*)(_qimage.constBits()),_qimage.bytesPerLine());
        break;
    case QImage::Format_RGB888: // 8-bit, 3 channel，RGB顺序
        _mat = cv::Mat(_qimage.height(),_qimage.width(),CV_8UC3, (void*)(_qimage.constBits()),_qimage.bytesPerLine());
        cv::cvtColor(this->_mat, this->_mat, cv::COLOR_RGB2BGR);// opencv以BGR顺序存储，需要转换顺序
        break;
    case QImage::Format_RGBA64: // uint64存储，顺序和Format_ARGB32相反，RGBA
        _mat = cv::Mat(_qimage.height(),_qimage.width(),CV_16UC4, (void*)(_qimage.constBits()),_qimage.bytesPerLine());
        cv::cvtColor(this->_mat, this->_mat, cv::COLOR_RGBA2BGRA);// opencv以BGRA顺序存储，需要转换顺序
        break;
    default:
        qWarning("QImage type not handled in switch: ", this->_qimage.format());
        break;
    }
}


QImage ImageUtils::MatToQImage(const cv::Mat &mat)
{
    // 判断图像类型
    switch (mat.type())
    {
    // 8-bit, 4 channel
    case CV_8UC4:
    {
        QImage image(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_ARGB32);
        return image.copy(); // 必须返回拷贝，否则 mat销毁后 image内数据不可访问，程序崩溃（QImage重载了operator=使用引用参数）
    }
    // 8-bit, 3 channel
    case CV_8UC3:
    {
        QImage image(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
        return image.rgbSwapped(); // 需要交换RGB通道
    }
    // 8-bit, 1 channel
    case CV_8UC1:
    {
        QImage image(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale8);
        return image.copy();
    }
    default:
        qWarning("cv::Mat image type not handled in switch: %d", mat.type());
        break;
    }

    return QImage();
}

cv::Mat ImageUtils::QImageToMat(const QImage &image)
{
    cv::Mat mat; // Mat构造：行数，列数，存储结构，数据，step每行多少字节
    switch(image.format())
    {
    case QImage::Format_Grayscale8: // 8-bit, 1 channel 灰度图
        mat = cv::Mat(image.height(), image.width(), CV_8UC1, (void*)image.constBits(), image.bytesPerLine());
        break;
    case QImage::Format_ARGB32: // uint32存储0xAARRGGBB，pc一般小端存储低位在前，实际字节顺序就成了BGRA
    case QImage::Format_RGB32: // Alpha为FF
    case QImage::Format_ARGB32_Premultiplied:
        mat = cv::Mat(image.height(), image.width(), CV_8UC4, (void*)image.constBits(), image.bytesPerLine());
        break;
    case QImage::Format_RGB888: // 8-bit, 3 channel，RGB顺序
        mat = cv::Mat(image.height(), image.width(), CV_8UC3, (void*)image.constBits(), image.bytesPerLine());
        cv::cvtColor(mat, mat, cv::COLOR_RGB2BGR);// opencv以BGR顺序存储，需要转换顺序
        break;
    case QImage::Format_RGBA64: // uint64存储，顺序和Format_ARGB32相反，RGBA
        mat = cv::Mat(image.height(), image.width(), CV_16UC4, (void*)image.constBits(), image.bytesPerLine());
        cv::cvtColor(mat, mat, cv::COLOR_RGBA2BGRA);// opencv以BGRA顺序存储，需要转换顺序
        break;
    default:
        qWarning("QImage type not handled in switch: ", image.format());
        break;
    }
    return mat;
}

cv::Mat ImageUtils::QImageToMat(const QImage &iamge, const char *format)
{
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    iamge.save(&buffer, format);
    buffer.close();
    return cv::imdecode(std::vector<char>(byteArray.begin(), byteArray.end()), cv::IMREAD_COLOR);
}

cv::Mat ImageUtils::LoadResourceImageToMat(const QString &path)
{
    QFile file(path);
    if(file.open(QFile::ReadOnly))
    {
        QByteArray ba = file.readAll();
        file.close();
        return cv::imdecode(std::vector<char>(ba.begin(), ba.end()), cv::IMREAD_COLOR);
    }
    return cv::Mat();
}

cv::Mat ImageUtils::GaussianBlur(const cv::Mat &MatImg, int radius)
{
    if(radius <= 0)
    {
        qWarning()<<"The blur radius must be greater than 0";
        return MatImg;
    }
    int ksize = radius * 2 + 1;
    cv::GaussianBlur(MatImg, MatImg, cv::Size(ksize,ksize), 0, 0);
    return MatImg;
}

QImage ImageUtils::GaussianBlur(const QImage &img, int radius)
{
    return MatToQImage(GaussianBlur(QImageToMat(img),radius));
}

QPixmap ImageUtils::GaussianBlur(const QPixmap &img, int radius)
{
    return QPixmap::fromImage(MatToQImage(GaussianBlur(QImageToMat(img.toImage()),radius)));
}

QPixmap ImageUtils::grabPixmap(QWidget *widget, const QRect &rect)
{
    QScreen *screen = QApplication::screenAt(widget->window()->pos());
    if (!screen)
        screen = QApplication::screens().at(0);
    return screen->grabWindow(0, rect.x(), rect.y(), rect.width(), rect.height());
}

ImageUtils::ImageUtils() {}
