#pragma once

#include <QMainWindow>
#include <QPointer>

class GalleryWindow;
class WorkspaceReference;

class FoundationWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit FoundationWindow(QWidget* parent = nullptr);

    void openGallery();
    void openReference();

private:
    void showAbout();

    QPointer<GalleryWindow> gallery_;
    QPointer<WorkspaceReference> reference_;
};
