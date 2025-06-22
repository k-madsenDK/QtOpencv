#include "annotationparser.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

bool AnnotationParser::loadFromFile(const QString& fileName)
{
    frameMap.clear();
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QTextStream in(&file);
    FrameAnnotations currentFrame;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.startsWith("Frame count:")) {
            // If we already have a frame, store it
            if (currentFrame.frameNumber >= 0)
                frameMap[currentFrame.frameNumber] = currentFrame;

            // Parse frame info
            QRegularExpression re(R"(Frame count:\s*(\d+)\s+Width:\s*(\d+)\s+Heigth:\s*(\d+))");
            auto match = re.match(line);
            if (match.hasMatch()) {
                currentFrame = FrameAnnotations();
                currentFrame.frameNumber = match.captured(1).toInt();
                int w = match.captured(2).toInt();
                int h = match.captured(3).toInt();
                currentFrame.size = QSize(w, h);
                currentFrame.labels.clear();
            }
        } else if (line.startsWith("Label:")) {
            // Parse label info
            QRegularExpression re(R"(Label:\s*([^\s]+)\s+ID:\s*(\d+)\s+Confidence:\s*([\d.]+)\s+Detection count:\s*(\d+)\s+Position:\s*center=\(([\d.]+),\s*([\d.]+)\)\s+Bounds:\s*xmin=([\d.]+),\s*ymin=([\d.]+),\s*xmax=([\d.]+),\s*ymax=([\d.]+))");
            auto match = re.match(line);
            if (match.hasMatch()) {
                FrameLabel fl;
                fl.label = match.captured(1);
                fl.id = match.captured(2).toInt();
                fl.confidence = match.captured(3).toFloat();
                fl.detectionCount = match.captured(4).toInt();
                fl.centerX = match.captured(5).toFloat();
                fl.centerY = match.captured(6).toFloat();
                fl.xmin = match.captured(7).toFloat();
                fl.ymin = match.captured(8).toFloat();
                fl.xmax = match.captured(9).toFloat();
                fl.ymax = match.captured(10).toFloat();
                currentFrame.labels.push_back(fl);
            }
        }
        // Drop lines not matching the above
    }
    // Store the last frame
    if (currentFrame.frameNumber >= 0)
        frameMap[currentFrame.frameNumber] = currentFrame;

    return true;
}

const FrameAnnotations* AnnotationParser::getAnnotations(int frameNumber) const
{
    auto it = frameMap.find(frameNumber);
    if (it != frameMap.end())
        return &it.value();
    return nullptr;
}