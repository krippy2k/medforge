#pragma once

#include <QMainWindow>

class Banner;
class QAction;
class QDockWidget;
class SampleViewport;
class WorkspaceActions;

class WorkspaceWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit WorkspaceWindow(const QString& layoutFile, QWidget* parent = nullptr);

    QDockWidget* studiesDock() const;
    QDockWidget* propertiesDock() const;
    QDockWidget* tasksDock() const;
    QAction* showSampleAction() const;
    QAction* resetCameraAction() const;
    QAction* resetLayoutAction() const;
    SampleViewport* sampleViewport() const;
    WorkspaceActions* actions() const;
    Banner* errorBanner() const;

    void saveLayout();
    void resetLayout();

public slots:
    void showSample();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void buildMenus();
    void buildDocks();
    void buildViewports();
    void applyDefaultDockPlacement();
    void syncActions();
    void updateProperties();

    QString layoutFile_;
    WorkspaceActions* actions_ = nullptr;
    QDockWidget* studiesDock_ = nullptr;
    QDockWidget* propertiesDock_ = nullptr;
    QDockWidget* tasksDock_ = nullptr;
    QAction* showSampleAction_ = nullptr;
    QAction* resetCameraAction_ = nullptr;
    QAction* resetLayoutAction_ = nullptr;
    SampleViewport* sampleViewport_ = nullptr;
    Banner* errorBanner_ = nullptr;
    class QLabel* statusLabel_ = nullptr;
    class QLabel* propertiesBody_ = nullptr;
    class QLabel* researchLabel_ = nullptr;
};
