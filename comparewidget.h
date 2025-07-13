#ifndef COMPAREWIDGET_H
#define COMPAREWIDGET_H

#include <QWidget>
#include <QMap>
#include <QSet>
#include <QString>
#include "annotationparser.h"

class QLineEdit;
class QPushButton;
class QListWidget;
class QTableWidget;
class QLabel;

class CompareWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CompareWidget(QWidget *parent = nullptr);

private slots:
    void browseFile1();
    void browseFile2();
    void loadFiles();
    void onCompare();

    //void onEndButtonClicked();        // <-- Add this

private:
    void populateLabelSelection();
    void showComparisonResults();

    QLineEdit *fileEdit1;
    QLineEdit *fileEdit2;
    QPushButton *browseButton1;
    QPushButton *browseButton2;
    QListWidget *labelListWidget;
    QPushButton *compareButton;
    QTableWidget *resultTable;
    QLabel *statusLabel;
    QPushButton *endButton;           // <-- Add this

    AnnotationParser parser1;
    AnnotationParser parser2;

    QSet<QString> allLabels;
    bool file1Loaded = false;
    bool file2Loaded = false;
};

#endif // COMPAREWIDGET_H
