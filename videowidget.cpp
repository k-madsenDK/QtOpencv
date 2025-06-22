#include "videowidget.h"
#include <QLabel>
#include <QVBoxLayout>
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
      fps(30),
      playing(false),
      currentFrameIdx(0)
{
    label->setAlignment(Qt::AlignCenter);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(label);
    setLayout(layout);

    connect(timer, &QTimer::timeout, this, &VideoWidget::timerNextFrame);
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
    annotationFrameSize = QSize();

    // Load annotation file with same base name but .txt extension
    QFileInfo fi(filePath);
    QString annotationFile = fi.path() + "/" + fi.completeBaseName() + ".txt";
    annotationParser.loadFromFile(annotationFile);

    if (!cap.isOpened()) {
        label->setText("Cannot open video file!");
        playing = false;
        emit playStateChanged(false);
        return;
    }

    fps = static_cast<int>(cap.get(cv::CAP_PROP_FPS));
    if (fps <= 0) fps = 30;
    currentFrameIdx = 0;
    cap.set(cv::CAP_PROP_POS_FRAMES, 0);
    nextFrame();
    play();
}

void VideoWidget::play()
{
    if (!cap.isOpened() || playing)
        return;

    playing = true;
    timer->start(1000 / fps);
    emit playStateChanged(true);
}

void VideoWidget::pause()
{
    if (!playing)
        return;

    playing = false;
    timer->stop();
    emit playStateChanged(false);
}

bool VideoWidget::isPlaying() const
{
    return playing;
}

void VideoWidget::timerNextFrame()
{
    nextFrame();
}

void VideoWidget::nextFrame()
{
    if (!cap.isOpened())
        return;

    cv::Mat frame;
    if (cap.get(cv::CAP_PROP_POS_FRAMES) != 0)
        currentFrameIdx = static_cast<int>(cap.get(cv::CAP_PROP_POS_FRAMES));
    if (!cap.read(frame) || frame.empty()) {
        label->setText("End of video or failed to read frame.");
        playing = false;
        emit playStateChanged(false);
        return;
    }
    currentFrameIdx = static_cast<int>(cap.get(cv::CAP_PROP_POS_FRAMES)) - 1;
    currentFrameOrig = frame.clone();
    showFrame(frame, currentFrameIdx);
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
}

void VideoWidget::showFrame(const cv::Mat& frame, int frameIdx)
{
    cv::Mat displayFrame = frame.clone();
    QSize frameSize = QSize(frame.cols, frame.rows);

    // Draw annotation overlays (if any)
    const FrameAnnotations* ann = annotationParser.getAnnotations(frameIdx);
    if (ann) {
        frameSize = ann->size; // Use annotation size for display
        for (const FrameLabel& fl : ann->labels) {
            int w = ann->size.width();
            int h = ann->size.height();
            /*int x1 = int(fl.xmin * w);
            int y1 = int(fl.ymin * h);
            int x2 = int(fl.xmax * w);
            int y2 = int(fl.ymax * h);*/
            int x_min = static_cast<int>(fl.xmin * frame.cols);
            int y_min = static_cast<int>(fl.ymin * frame.rows);
            int x_max = static_cast<int>(fl.xmax * frame.cols);
            int y_max = static_cast<int>(fl.ymax * frame.rows);
            // Draw rectangle
            cv::rectangle(displayFrame, cv::Point(x_min, y_min), cv::Point(x_max, y_max), cv::Scalar(255,255,255), 2);
            // Draw label + confidence above box
            std::stringstream stream;
            stream << std::fixed << std::setprecision(2) << fl.confidence;
            std::string conf = stream.str();
            std::string labelText = fl.label.toStdString() + " " + conf;//.arg(fl.confidence);
            int baseLine = 0;
            cv::Size textSize = cv::getTextSize(labelText, cv::FONT_HERSHEY_SIMPLEX, 1, 1, &baseLine);
            int textY = std::max(y_min - 10, textSize.height + 2);
            cv::putText(displayFrame, labelText, cv::Point(x_min, textY),
                        cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255,255,255), 2);
        }
        annotationFrameSize = ann->size;
    } else {
        annotationFrameSize = QSize(frame.cols, frame.rows);
    }

    // Convert to QImage and display at annotation size!
    cv::Mat rgb;
    cv::cvtColor(displayFrame, rgb, cv::COLOR_BGR2RGB);

    QImage img(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888);

    // Scale to annotation size, not widget size
    QPixmap pixmap = QPixmap::fromImage(img).scaled(annotationFrameSize,
        Qt::KeepAspectRatio, Qt::SmoothTransformation);

    label->setPixmap(pixmap);

    emit frameInfoChanged(frameIdx, annotationFrameSize);
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

void VideoWidget::closeEvent(QCloseEvent *event)
{
    timer->stop();
    if (cap.isOpened())
        cap.release();
    QWidget::closeEvent(event);
}

void VideoWidget::resizeEvent(QResizeEvent *) {
    // re-display to update scaling
    if (!currentFrameOrig.empty())
        showFrame(currentFrameOrig, currentFrameIdx);
}
