#pragma once

#include "medforge/core/CommandHistory.h"
#include "medforge/geometry/Measure.h"
#include "medforge/geometry/Surface.h"
#include "medforge/imaging/CtImport.h"
#include "medforge/imaging/LabelMap.h"
#include "medforge/rendering/Isosurface.h"

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
class VolumeViewport;
class QCheckBox;
class QLineEdit;
class QListWidget;
class QPushButton;
class QTableWidget;
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
    VolumeViewport* volumeViewport() const;
    QLabel* meshStatsLabel() const;
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
    void buildSurface(int delayBeforeMs = 0);
    void cancelSurface();
    void applySegmentThreshold();
    void growFromCrosshair(int delayBeforeMs = 0);
    void cancelSegment();
    void paintSegment(medforge::imaging::MprPlane plane, int x, int y);
    void undoSegment();
    void redoSegment();
    bool saveMask(const QString& path, QString* error);
    bool loadMask(const QString& path, QString* error);
    void buildLabelMesh(int delayBeforeMs = 0);
    void focusLabeledVoxel();
    void activateMeasureAtCrosshair();
    int placeLandmarkAtVoxel(int i, int j, int k);
    int landmarkCount() const;
    void selectLandmark(int index);
    void deleteSelectedLandmark();
    void moveSelectedLandmark(double lMm, double pMm, double sMm);
    int defineDistance(int first, int second);
    double measureValue(int index) const;
    QString measureUnit(int index) const;
    void undoMeasure();
    void redoMeasure();
    bool exportMeasureReport(const QString& path, QString* error);
    bool saveMeasures(const QString& path, QString* error);
    bool loadMeasures(const QString& path, QString* error);
    bool segmentUnsaved() const;
    int labelAt(int i, int j, int k) const;

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
    void shutdownSurface();
    void applySurface(int generation, const medforge::rendering::SurfaceResult& result);
    void refreshSurfaceView();
    void showVolumePane(bool visible);
    void resetSegment();
    void commitSegmentEdit(medforge::imaging::LabelEdit edit, const char* name);
    void refreshSegmentStatus();
    void commitMeasures(medforge::geometry::MeasureDocument next);
    void refreshMeasures();
    void resetMeasures();
    void updateMeasureOverlays();
    int nearestLandmark() const;
    void saveSegmentTools();
    void loadSegmentTools();
    void shutdownSegment();
    void applyGrow(int generation, const medforge::imaging::SegmentResult& result);
    void paintPlane(MprViewport* view, medforge::imaging::MprPlane plane, int index);
    void handlePlanePointer(medforge::imaging::MprPlane plane, int x, int y);

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
    VolumeViewport* volumeViewport_ = nullptr;
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
    SegmentedControl* volumePresets_ = nullptr;
    SegmentedControl* cameraViews_ = nullptr;
    QPushButton* volumeVisibleButton_ = nullptr;
    QPushButton* meshVisibleButton_ = nullptr;
    QPushButton* clipButton_ = nullptr;
    QPushButton* buildSurfaceButton_ = nullptr;
    NumericEditor* isoThreshold_ = nullptr;
    QLabel* meshStats_ = nullptr;
    BrowserRow* surfaceRow_ = nullptr;
    BrowserRow* segmentRow_ = nullptr;
    SegmentedControl* segmentTool_ = nullptr;
    SegmentedControl* segmentLabel_ = nullptr;
    SegmentedControl* segmentConnectivity_ = nullptr;
    NumericEditor* segmentMin_ = nullptr;
    NumericEditor* segmentMax_ = nullptr;
    NumericEditor* segmentRadius_ = nullptr;
    QCheckBox* segmentLimitSlice_ = nullptr;
    QCheckBox* segmentPreviewToggle_ = nullptr;
    QLabel* segmentPreview_ = nullptr;
    QLabel* segmentUnsaved_ = nullptr;
    QLabel* segmentProvenance_ = nullptr;
    QPushButton* applyThresholdButton_ = nullptr;
    QPushButton* regionGrowButton_ = nullptr;
    QPushButton* labelMeshButton_ = nullptr;
    QAction* undoSegmentAction_ = nullptr;
    QAction* redoSegmentAction_ = nullptr;
    TaskProgress* taskProgress_ = nullptr;
    QLabel* taskIdleLabel_ = nullptr;
    std::thread importThread_;
    std::thread surfaceThread_;
    medforge::core::CancellationSource importCancel_;
    medforge::core::CancellationSource surfaceCancel_;
    int surfaceGeneration_ = 0;
    medforge::geometry::TriangleMesh surfaceMesh_;
    medforge::rendering::SurfaceResult surfaceResult_;
    bool hasSurface_ = false;
    medforge::imaging::CtVolume volume_;
    medforge::imaging::LabelMap labelMap_;
    medforge::core::CommandHistory segmentHistory_;
    int segmentRevision_ = 0;
    int segmentSavedRevision_ = 0;
    std::thread segmentThread_;
    medforge::core::CancellationSource segmentCancel_;
    int segmentGeneration_ = 0;
    bool labelMeshRequest_ = false;
    medforge::geometry::MeasureDocument measures_;
    medforge::core::CommandHistory measureHistory_;
    int selectedLandmark_ = -1;
    std::vector<int> measurePending_;
    bool updatingMeasure_ = false;
    SegmentedControl* measureTool_ = nullptr;
    QCheckBox* measureSnap_ = nullptr;
    NumericEditor* measureSnapTolerance_ = nullptr;
    NumericEditor* measureL_ = nullptr;
    NumericEditor* measureP_ = nullptr;
    NumericEditor* measureS_ = nullptr;
    QLineEdit* measureName_ = nullptr;
    QTableWidget* measureTable_ = nullptr;
    QListWidget* landmarkList_ = nullptr;
    QLabel* measureNotice_ = nullptr;
    medforge::imaging::MprCrosshair crosshair_;
    medforge::imaging::WindowLevel windowLevel_ = medforge::imaging::WindowLevel::softTissue();
    bool hasVolume_ = false;
    bool updatingWindowLevel_ = false;
    bool closing_ = false;
};
