#include "mainwindow.h"
#include "videowidget.h"
#include "comparewidget.h" // <-- Add this include
#include <QMenuBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QAction>
#include <QKeyEvent>
#include <QFileInfo>
#include <QLabel>        // <-- THIS LINE IS NEEDED


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setMinimumSize(800, 600);
    setWindowTitle("Mainframe Window");

    // Menu bar
    QMenu *fileMenu = menuBar()->addMenu("&File");
    QAction *openAction = fileMenu->addAction("&Open MP4...");
    connect(openAction, &QAction::triggered, this, [this]() {
        QString fileName = QFileDialog::getOpenFileName(this, "Open MP4 File", QString(), "Video Files (*.mp4)");
        if (!fileName.isEmpty()) {
            videoWidget->setVideo(fileName);
        }
    });

    QAction *saveFrameAction = fileMenu->addAction("&Frame Save...");
    saveFrameAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_S));
    connect(saveFrameAction, &QAction::triggered, this, &MainWindow::saveFrame);

    //QAction *compareAction = fileMenu->addAction("&Compare Annotation Files...");
    //connect(compareAction, &QAction::triggered, this, &MainWindow::showCompareDialog); // <-- Add this
    QAction *compareAction = fileMenu->addAction("&Compare Annotation Files...");
    connect(compareAction, &QAction::triggered, this, &MainWindow::showCompareWindow);

    fileMenu->addSeparator();
    fileMenu->addAction("E&xit", this, SLOT(close()));

    statusBar()->showMessage("Stopped");

    videoWidget = new VideoWidget(this);
    setCentralWidget(videoWidget);
    connect(videoWidget, &VideoWidget::playStateChanged, this, &MainWindow::updateStatusBar);
    connect(videoWidget, &VideoWidget::frameInfoChanged, this, &MainWindow::showFrameInfo);
    connect(videoWidget, &VideoWidget::frameSaved, this, &MainWindow::showFrameSaved);
}

MainWindow::~MainWindow()
{
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Space:
        if (videoplay) {
            videoplay = false;
            videoWidget->pause();
        } else {
            videoplay = true;
            videoWidget->play();
        }
        event->accept();
        break;
    case Qt::Key_Left:
        videoWidget->prevFrame();
        event->accept();
        break;
    case Qt::Key_Right:
        videoWidget->nextFrame();
        event->accept();
        break;
    default:
        QMainWindow::keyPressEvent(event);
    }
}

void MainWindow::updateStatusBar(bool playing)
{
    statusBar()->showMessage(playing ? "Playing" : "Stopped");
}

void MainWindow::showFrameInfo(int frameNumber, QSize size)
{
    statusBar()->showMessage(
        QString("Frame: %1, Size: %2x%3")
            .arg(frameNumber, 6, 10, QChar('0'))
            .arg(size.width())
            .arg(size.height())
    );
}

void MainWindow::showFrameSaved(const QString &filename)
{
    if (!filename.isEmpty())
        statusBar()->showMessage(QFileInfo(filename).fileName() + " Saved");
    else
        statusBar()->showMessage("No frame to save!");
}

void MainWindow::saveFrame()
{
    videoWidget->saveCurrentFrame();
}

/*void MainWindow::showCompareDialog()
{
    if (!compareWidget) {
        compareWidget = new CompareWidget(this);
        compareWidget->setAttribute(Qt::WA_DeleteOnClose, true);
        connect(compareWidget, &QObject::destroyed, [this]() { compareWidget = nullptr; });
    }
    compareWidget->show();
    compareWidget->raise();
    compareWidget->activateWindow();
}*/

void MainWindow::showCompareWindow()
{
    // Create CompareWidget as a top-level window
    CompareWidget *compareWin = new CompareWidget(nullptr);
    compareWin->setAttribute(Qt::WA_DeleteOnClose); // Auto-delete on close
    compareWin->resize(1200, 800); // Or appropriate size for your table
    compareWin->show();
}

void VideoWidget::pause()
{
    if (!playing)
        return;

    playing = false;
    timer->stop();
    emit playStateChanged(false);
}
void VideoWidget::play()
{
    if (!cap.isOpened() || playing)
        return;

    playing = true;
    timer->start(1000 / fps);
    emit playStateChanged(true);
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
