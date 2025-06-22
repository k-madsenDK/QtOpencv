#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSize>

class VideoWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    VideoWidget* videoWidget;
    bool videoplay = false;

private slots:
    void updateStatusBar(bool playing);
    void showFrameInfo(int frameNumber, QSize size);
    void showFrameSaved(const QString &filename);
    void saveFrame();
};

#endif // MAINWINDOW_H
