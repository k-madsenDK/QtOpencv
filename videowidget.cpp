#include "videowidget.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QSlider>
#include <QImage>
#include <QCloseEvent>
#include <QFileInfo>
#include <QDir>
#include <QPainter>
#include <QFont>
#include <sstream>

VideoWidget::VideoWidget(QWidget *parent)
    : QWidget(parent),
      timer(new QTimer(this)),
      label(new QLabel(this)),
      frameSlider(new QSlider(Qt::Horizontal, this)),
      fps(30),
      playing(false),
      currentFrameIdx(0),
      totalFrames(0)
{
    label->setAlignment(Qt::AlignCenter);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(label);
    layout->addWidget(frameSlider);
    setLayout(layout);

    connect(timer, &QTimer::timeout, this, &VideoWidget::timerNextFrame);

    frameSlider->setMinimum(0);
    frameSlider->setMaximum(0);
    frameSlider->setValue(0);
    frameSlider->setEnabled(false);

    connect(frameSlider, &QSlider::valueChanged, this, &VideoWidget::setFrameFromSlider);
}

VideoWidget::~VideoWidget()
{
    timer->stop();
    if (cap.isOpened())
        cap.release();
}

void VideoWidget::setVideo(const QString &filePath)
{
    if (cap.isOpened())
        cap.release();

    cap.open(filePath.toStdString());
    loadedFile = filePath;

    // Load annotation file (assume .txt extension)
    QString annotFile = filePath;
    annotFile.chop(4); // Remove ".mp4"
    annotFile += ".txt";
    bool annLoaded = annotationParser.loadFromFile(annotFile);
   /* if (!annLoaded) {
        qDebug() << "Annotation file not loaded:" << annotFile;
        // Optionally show a message or fallback behavior
    }*/

    if (!cap.isOpened()) {
        label->setText("Failed to open video.");
        frameSlider->setEnabled(false);
        return;
    }

    fps = static_cast<int>(cap.get(cv::CAP_PROP_FPS));
    totalFrames = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_COUNT));

    setSliderRange();

    currentFrameIdx = 0;
    frameSlider->setValue(0);

    cv::Mat frame;
    if (cap.read(frame) && !frame.empty()) {
        currentFrameOrig = frame.clone();
        showFrame(frame, currentFrameIdx);
        frameSlider->setEnabled(true);
    } else {
        label->setText("Failed to read first frame.");
        frameSlider->setEnabled(false);
    }
}

void VideoWidget::setSliderRange()
{
    frameSlider->setMinimum(0);
    frameSlider->setMaximum(totalFrames > 0 ? totalFrames - 1 : 0);
}

void VideoWidget::setFrameFromSlider(int frameNumber)
{
    if (!cap.isOpened())
        return;

    if (playing) pause();

    if (frameNumber == currentFrameIdx)
        return;

    cap.set(cv::CAP_PROP_POS_FRAMES, frameNumber);
    cv::Mat frame;
    if (!cap.read(frame) || frame.empty()) {
        label->setText("Failed to read frame.");
        return;
    }
    currentFrameIdx = frameNumber;
    currentFrameOrig = frame.clone();
    showFrame(frame, currentFrameIdx);
}

void VideoWidget::nextFrame()
{
    if (!cap.isOpened())
        return;

    //if (playing) pause();

    int goTo = currentFrameIdx + 1;
    if (goTo >= totalFrames)
        goTo = totalFrames - 1;
    cap.set(cv::CAP_PROP_POS_FRAMES, goTo);
    cv::Mat frame;
    if (!cap.read(frame) || frame.empty()) {
        label->setText("Failed to read frame.");
        return;
    }
    currentFrameIdx = goTo;
    currentFrameOrig = frame.clone();
    showFrame(frame, currentFrameIdx);
    updateSlider();
}

void VideoWidget::prevFrame()
{
    if (!cap.isOpened())
        return;

    if (playing) pause();

    int goTo = currentFrameIdx - 1;
    if (goTo < 0)
        goTo = 0;
    cap.set(cv::CAP_PROP_POS_FRAMES, goTo);
    cv::Mat frame;
    if (!cap.read(frame) || frame.empty()) {
        label->setText("Failed to read frame.");
        return;
    }
    currentFrameIdx = goTo;
    currentFrameOrig = frame.clone();
    showFrame(frame, currentFrameIdx);
    updateSlider();
}

void VideoWidget::updateSlider()
{
    frameSlider->blockSignals(true);
    frameSlider->setValue(currentFrameIdx);
    frameSlider->blockSignals(false);
}

void VideoWidget::saveCurrentFrame()
{
    if (currentFrameOrig.empty()) {
        emit frameSaved(QString());
        return;
    }

    QFileInfo fi(loadedFile);
    QString baseName = fi.completeBaseName();
    QString defaultName = QString("%1_%2.jpg").arg(baseName).arg(currentFrameIdx, 6, 10, QChar('0'));

    QString outFile = QDir::current().absoluteFilePath(defaultName);
    cv::imwrite(outFile.toStdString(), currentFrameOrig); // BGR!
    emit frameSaved(outFile);
}

void VideoWidget::timerNextFrame()
{
    nextFrame();
}

void VideoWidget::resizeEvent(QResizeEvent *event)
{
    // Re-display to update scaling
    if (!currentFrameOrig.empty())
        showFrame(currentFrameOrig, currentFrameIdx);
    QWidget::resizeEvent(event);
}

void VideoWidget::closeEvent(QCloseEvent *event)
{
    timer->stop();
    if (cap.isOpened())
        cap.release();
    QWidget::closeEvent(event); // call base class event handler
}
