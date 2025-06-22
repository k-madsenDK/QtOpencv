#include "mainwindow.h"
#include "videowidget.h"
#include <QMenuBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QAction>
#include <QKeyEvent>
#include <QFileInfo>

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
