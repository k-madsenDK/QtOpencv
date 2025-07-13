#include "comparewidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QSet>
#include <QFileInfo>

CompareWidget::CompareWidget(QWidget *parent)
    : QWidget(parent)
{
    // UI elements
    fileEdit1 = new QLineEdit(this);
    fileEdit2 = new QLineEdit(this);
    browseButton1 = new QPushButton("Browse...", this);
    browseButton2 = new QPushButton("Browse...", this);
    labelListWidget = new QListWidget(this);
    labelListWidget->setSelectionMode(QAbstractItemView::MultiSelection);
    compareButton = new QPushButton("Compare", this);
    resultTable = new QTableWidget(this);
    statusLabel = new QLabel(this);

    // Layouts
    QHBoxLayout *fileLayout1 = new QHBoxLayout;
    fileLayout1->addWidget(new QLabel("File 1:", this));
    fileLayout1->addWidget(fileEdit1);
    fileLayout1->addWidget(browseButton1);

    QHBoxLayout *fileLayout2 = new QHBoxLayout;
    fileLayout2->addWidget(new QLabel("File 2:", this));
    fileLayout2->addWidget(fileEdit2);
    fileLayout2->addWidget(browseButton2);

    QVBoxLayout *leftLayout = new QVBoxLayout;
    leftLayout->addLayout(fileLayout1);
    leftLayout->addLayout(fileLayout2);
    leftLayout->addWidget(new QLabel("Labels:", this));
    leftLayout->addWidget(labelListWidget);
    leftLayout->addWidget(compareButton);
    leftLayout->addWidget(statusLabel);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addLayout(leftLayout, 1);
    hLayout->addWidget(resultTable, 3);
    mainLayout->addLayout(hLayout);
    setLayout(mainLayout);

    // Table setup
    resultTable->setColumnCount(5);
    QStringList headers = {"Frame", "Label", "File 1 Confidence", "File 2 Confidence", "Change"};
    resultTable->setHorizontalHeaderLabels(headers);
    resultTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    statusLabel->setText("Select files and labels to compare.");

    // Connections
    connect(browseButton1, &QPushButton::clicked, this, &CompareWidget::browseFile1);
    connect(browseButton2, &QPushButton::clicked, this, &CompareWidget::browseFile2);
    connect(fileEdit1, &QLineEdit::editingFinished, this, &CompareWidget::loadFiles);
    connect(fileEdit2, &QLineEdit::editingFinished, this, &CompareWidget::loadFiles);
    connect(compareButton, &QPushButton::clicked, this, &CompareWidget::onCompare);
    //connect(endButton, &QPushButton::clicked, this, &CompareWidget::onEndButtonClicked);

    setWindowTitle("Compare Annotation Files");
    resize(1000, 600);
}

void CompareWidget::browseFile1()
{
    QString fn = QFileDialog::getOpenFileName(this, "Select annotation file 1", QString(), "Text Files (*.txt)");
    if (!fn.isEmpty()) {
        fileEdit1->setText(fn);
        loadFiles();
    }
}

void CompareWidget::browseFile2()
{
    QString fn = QFileDialog::getOpenFileName(this, "Select annotation file 2", QString(), "Text Files (*.txt)");
    if (!fn.isEmpty()) {
        fileEdit2->setText(fn);
        loadFiles();
    }
}

void CompareWidget::loadFiles()
{
    QString f1 = fileEdit1->text().trimmed();
    QString f2 = fileEdit2->text().trimmed();
    bool ok1 = false, ok2 = false;
    if (!f1.isEmpty()) ok1 = parser1.loadFromFile(f1);
    if (!f2.isEmpty()) ok2 = parser2.loadFromFile(f2);

    file1Loaded = ok1;
    file2Loaded = ok2;

    if (ok1 && ok2) {
        statusLabel->setText("Both files loaded. Select labels and press Compare.");
        populateLabelSelection();
    } else if (ok1) {
        statusLabel->setText("File 1 loaded. Select file 2.");
    } else if (ok2) {
        statusLabel->setText("File 2 loaded. Select file 1.");
    } else {
        statusLabel->setText("Failed to load files.");
    }
}

void CompareWidget::populateLabelSelection()
{
    allLabels.clear();
    labelListWidget->clear();

    // Collect all labels from both files
    for (const auto &frame : parser1.frameMap) {
        for (const FrameLabel &fl : frame.labels) {
            allLabels.insert(fl.label);
        }
    }
    for (const auto &frame : parser2.frameMap) {
        for (const FrameLabel &fl : frame.labels) {
            allLabels.insert(fl.label);
        }
    }
    // Fill list widget
    for (const QString &label : allLabels) {
        QListWidgetItem *item = new QListWidgetItem(label, labelListWidget);
        item->setCheckState(Qt::Unchecked);
    }
}

void CompareWidget::onCompare()
{
    if (!file1Loaded || !file2Loaded) {
        QMessageBox::warning(this, "Error", "Both files must be loaded!");
        return;
    }
    // Get selected labels
    QSet<QString> selectedLabels;
    for (int i = 0; i < labelListWidget->count(); ++i) {
        QListWidgetItem *item = labelListWidget->item(i);
        if (item->checkState() == Qt::Checked)
            selectedLabels.insert(item->text());
    }
    if (selectedLabels.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please select at least one label.");
        return;
    }

    // Find all frame numbers in either file
    QSet<int> allFrames;
    for (auto it = parser1.frameMap.begin(); it != parser1.frameMap.end(); ++it)
        allFrames.insert(it.key());
    for (auto it = parser2.frameMap.begin(); it != parser2.frameMap.end(); ++it)
        allFrames.insert(it.key());

    QList<int> frameList = allFrames.values();
    std::sort(frameList.begin(), frameList.end());

    resultTable->setRowCount(0);

    int nRows = 0;
    for (int frameNum : frameList) {
        // For each selected label
        for (const QString &label : selectedLabels) {
            // Get highest confidence per file
            float conf1 = -1.0f, conf2 = -1.0f;
            const FrameAnnotations* ann1 = parser1.getAnnotations(frameNum);
            if (ann1) {
                for (const FrameLabel &fl : ann1->labels) {
                    if (fl.label == label)
                        conf1 = std::max(conf1, fl.confidence);
                }
            }
            const FrameAnnotations* ann2 = parser2.getAnnotations(frameNum);
            if (ann2) {
                for (const FrameLabel &fl : ann2->labels) {
                    if (fl.label == label)
                        conf2 = std::max(conf2, fl.confidence);
                }
            }
            // If neither file has that label in this frame, skip
            if (conf1 < 0 && conf2 < 0)
                continue;
            // Show row
            resultTable->insertRow(nRows);

            QTableWidgetItem *frameItem = new QTableWidgetItem(QString::number(frameNum));
            QTableWidgetItem *labelItem = new QTableWidgetItem(label);
            QTableWidgetItem *conf1Item = new QTableWidgetItem(conf1 < 0 ? "-" : QString::number(conf1, 'f', 2));
            QTableWidgetItem *conf2Item = new QTableWidgetItem(conf2 < 0 ? "-" : QString::number(conf2, 'f', 2));
            float diff = 0.0f;
            QString diffStr = "-";
            if (conf1 >= 0 && conf2 >= 0) {
                diff = conf2 - conf1;
                diffStr = (diff >= 0 ? "+" : "") + QString::number(diff, 'f', 2);
            }
            QTableWidgetItem *diffItem = new QTableWidgetItem(diffStr);

            // Highlight improvement or regression
            if (conf1 >= 0 && conf2 >= 0) {
                if (diff > 0)
                    diffItem->setBackground(Qt::green);
                else if (diff < 0)
                    diffItem->setBackground(Qt::red);
            }

            resultTable->setItem(nRows, 0, frameItem);
            resultTable->setItem(nRows, 1, labelItem);
            resultTable->setItem(nRows, 2, conf1Item);
            resultTable->setItem(nRows, 3, conf2Item);
            resultTable->setItem(nRows, 4, diffItem);
            ++nRows;
        }
    }
    statusLabel->setText(QString("Compared %1 frames.").arg(nRows));
}


