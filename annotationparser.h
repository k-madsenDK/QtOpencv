#ifndef ANNOTATIONPARSER_H
#define ANNOTATIONPARSER_H

#include <QString>
#include <QSize>
#include <QMap>
#include <vector>

struct FrameLabel {
    QString label;
    int id;
    float confidence;
    int detectionCount;
    float centerX, centerY;
    float xmin, ymin, xmax, ymax;
    
    // Utility function to calculate center from bounds
    // Useful if center is not provided in input data
    void calculateCenterFromBounds() {
        centerX = (xmin + xmax) / 2.0f;
        centerY = (ymin + ymax) / 2.0f;
    }
};

struct FrameAnnotations {
    int frameNumber = -1;
    QSize size;
    std::vector<FrameLabel> labels;
};

class AnnotationParser
{
public:
    AnnotationParser() {}

    bool loadFromFile(const QString& fileName);

    // Returns annotation for frameNumber, or nullptr if not found
    const FrameAnnotations* getAnnotations(int frameNumber) const;
    QMap<int, FrameAnnotations> frameMap;

private:

};

#endif // ANNOTATIONPARSER_H
