#pragma once

#include "medforge/imaging/CtImport.h"

#include <QMainWindow>
#include <thread>

class Banner;
class BrowserRow;
class MprViewport;
class NumericEditor;
class QAction;
class QDockWidget;
class QLabel;
class SampleViewport;
class SegmentedControl;
class TaskProgress;
class WorkspaceActions;

class WorkspaceWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit WorkspaceWindow(const QString& layoutFile, QWidget* parent = nullptr);
    ~WorkspaceWindow() override;

    QDockWidget* studiesDock() const;
    QDockWidget* propertiesDock() const;
    QDockWidget* tasksDock() const;
    QAction* showSampleAction() const;
    QAction* resetCameraAction() const;
    QAction* resetLayoutAction() const;
    QAction* importCtAction() const;
    SampleViewport* sampleViewport() const;
    MprViewport* axialViewport() const;
    MprViewport* sagittalViewport() const;
    MprViewport* coronalViewport() const;
    WorkspaceActions* actions() const;
    Banner* errorBanner() const;
    QLabel* cursorReadout() const;

    void saveLayout();
    void resetLayout();
    void importFolder(const QString& folder, int delayPerFileMs = 0);
    void cancelImport();

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
    void refreshMpr();
    void applyImportResult(const medforge::imaging::ImportResult& result);
    void clearLoadedSeries();
    void shutdownImport();

    QString layoutFile_;
    WorkspaceActions* actions_ = nullptr;
    QDockWidget* studiesDock_ = nullptr;
    QDockWidget* propertiesDock_ = nullptr;
    QDockWidget* tasksDock_ = nullptr;
    QAction* showSampleAction_ = nullptr;
    QAction* resetCameraAction_ = nullptr;
    QAction* resetLayoutAction_ = nullptr;
    QAction* importAction_ = nullptr;
    SampleViewport* sampleViewport_ = nullptr;
    MprViewport* axialViewport_ = nullptr;
    MprViewport* sagittalViewport_ = nullptr;
    MprViewport* coronalViewport_ = nullptr;
    Banner* errorBanner_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QLabel* cursorLabel_ = nullptr;
    QLabel* propertiesBody_ = nullptr;
    QLabel* researchLabel_ = nullptr;
    BrowserRow* studiesRow_ = nullptr;
    BrowserRow* seriesRow_ = nullptr;
    QWidget* ctProperties_ = nullptr;
    SegmentedControl* windowPresets_ = nullptr;
    NumericEditor* windowWidth_ = nullptr;
    NumericEditor* windowCenter_ = nullptr;
    TaskProgress* taskProgress_ = nullptr;
    QLabel* taskIdleLabel_ = nullptr;
    std::thread importThread_;
    medforge::core::CancellationSource importCancel_;
    medforge::imaging::CtVolume volume_;
    medforge::imaging::MprCrosshair crosshair_;
    medforge::imaging::WindowLevel windowLevel_ = medforge::imaging::WindowLevel::softTissue();
    bool hasVolume_ = false;
    bool updatingWindowLevel_ = false;
    bool closing_ = false;
};
