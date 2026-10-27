#pragma once

#include "medforge/core/Cancellation.h"

#include <QList>
#include <QMainWindow>

class QCloseEvent;
class QGridLayout;
class QResizeEvent;
class TaskProgress;

class GalleryWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit GalleryWindow(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void relayout();
    void startSampleTask();
    void cancelSampleTask();
    void setSampleProgress(int percent);
    void finishSampleTask(bool cancelled);

    medforge::core::CancellationSource taskSource_;
    bool taskRunning_ = false;
    TaskProgress* taskProgress_ = nullptr;
    QWidget* content_ = nullptr;
    QGridLayout* grid_ = nullptr;
    QList<QWidget*> sections_;
    int columns_ = 0;
};
