#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QSlider>
#include <opencv2/opencv.hpp>
#include "annotationparser.h"

class QLabel;

class VideoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VideoWidget(QWidget *parent = nullptr);
    ~VideoWidget() override;

    void setVideo(const QString &filePath);
    void play();
    void pause();
    bool isPlaying() const;

public slots:
    void nextFrame();
    void prevFrame();
    void saveCurrentFrame();
    void setFrameFromSlider(int frameNumber);

signals:
    void playStateChanged(bool playing);
    void frameInfoChanged(int frameNumber, QSize size);
    void frameSaved(const QString &filename);

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void timerNextFrame();

private:
    void showFrame(const cv::Mat& frame, int frameIdx);

    cv::VideoCapture cap;
    QTimer *timer;
    QLabel *label;
    QSlider *frameSlider;
    int fps;
    bool playing;
    int currentFrameIdx;
    int totalFrames;
    cv::Mat currentFrameOrig;
    QString loadedFile;

    AnnotationParser annotationParser;
    QSize annotationFrameSize;
    void updateSlider();
    void setSliderRange();
};

#endif // VIDEOWIDGET_H
