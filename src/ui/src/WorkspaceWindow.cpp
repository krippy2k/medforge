#include "medforge/ui/WorkspaceWindow.h"

#include "medforge/core/Version.h"
#include "medforge/imaging/CtVolume.h"
#include "medforge/rendering/TransferFunction.h"
#include "medforge/ui/DesignTokens.h"
#include "medforge/ui/FoundationWidgets.h"
#include "medforge/ui/LayoutPreferences.h"
#include "medforge/ui/MprViewport.h"
#include "medforge/ui/NumericEditor.h"
#include "medforge/ui/SampleViewport.h"
#include "medforge/ui/SegmentedControl.h"
#include "medforge/ui/VolumeViewport.h"
#include "medforge/ui/WorkspaceActions.h"

#include <QAction>
#include <QCheckBox>
#include <QCloseEvent>
#include <QHBoxLayout>
#include <QDockWidget>
#include <QFileDialog>
#include <QFrame>
#include <QGridLayout>
#include <QImage>
#include <QKeySequence>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QFile>
#include <QScrollArea>
#include <QStatusBar>
#include <QTextStream>
#include <QToolBar>
#include <QVBoxLayout>
#include <cmath>
#include <filesystem>

namespace {

QDockWidget* makeDock(const QString& title, const QString& objectName, QWidget* parent) {
    auto* dock = new QDockWidget(title, parent);
    dock->setObjectName(objectName);
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea |
                          Qt::BottomDockWidgetArea);
    return dock;
}

std::filesystem::path folderPath(const QString& folder) {
#ifdef Q_OS_WIN
    return std::filesystem::path(folder.toStdWString());
#else
    return std::filesystem::path(folder.toStdString());
#endif
}

} // namespace

WorkspaceWindow::WorkspaceWindow(const QString& layoutFile, QWidget* parent)
    : QMainWindow(parent), layoutFile_(layoutFile), actions_(new WorkspaceActions(this)) {
    setWindowTitle(
        QStringLiteral("MedForge %1").arg(QString::fromUtf8(medforge::core::kMilestone)));
    setObjectName(QStringLiteral("workspaceWindow"));
    setMinimumSize(960, 640);
    resize(1440, 900);
    setDockNestingEnabled(true);

    buildMenus();
    buildDocks();
    buildViewports();
    applyDefaultDockPlacement();

    LayoutPreferences preferences(layoutFile_);
    QByteArray state;
    QByteArray geometry;
    if (preferences.load(&state, &geometry)) {
        restoreGeometry(geometry);
        restoreState(state);
    }
    propertiesDock_->setMinimumWidth(360);
    connect(actions_, &WorkspaceActions::phaseChanged, this, [this] {
        syncActions();
        updateProperties();
    });
    syncActions();
    updateProperties();
    sampleViewport_->realizeNativeWindow();
    volumeViewport_->realizeNativeWindow();
    loadSegmentTools();
}

WorkspaceWindow::~WorkspaceWindow() {
    shutdownSegment();
    shutdownSurface();
    shutdownImport();
}

QDockWidget* WorkspaceWindow::studiesDock() const {
    return studiesDock_;
}

QDockWidget* WorkspaceWindow::propertiesDock() const {
    return propertiesDock_;
}

QDockWidget* WorkspaceWindow::tasksDock() const {
    return tasksDock_;
}

QAction* WorkspaceWindow::showSampleAction() const {
    return showSampleAction_;
}

QAction* WorkspaceWindow::resetCameraAction() const {
    return resetCameraAction_;
}

QAction* WorkspaceWindow::resetLayoutAction() const {
    return resetLayoutAction_;
}

QAction* WorkspaceWindow::importCtAction() const {
    return importAction_;
}

SampleViewport* WorkspaceWindow::sampleViewport() const {
    return sampleViewport_;
}

VolumeViewport* WorkspaceWindow::volumeViewport() const {
    return volumeViewport_;
}

QLabel* WorkspaceWindow::meshStatsLabel() const {
    return meshStats_;
}

MprViewport* WorkspaceWindow::axialViewport() const {
    return axialViewport_;
}

MprViewport* WorkspaceWindow::sagittalViewport() const {
    return sagittalViewport_;
}

MprViewport* WorkspaceWindow::coronalViewport() const {
    return coronalViewport_;
}

WorkspaceActions* WorkspaceWindow::actions() const {
    return actions_;
}

Banner* WorkspaceWindow::errorBanner() const {
    return errorBanner_;
}

QLabel* WorkspaceWindow::cursorReadout() const {
    return cursorLabel_;
}

void WorkspaceWindow::saveLayout() {
    LayoutPreferences preferences(layoutFile_);
    preferences.save(saveState(), saveGeometry());
}

void WorkspaceWindow::resetLayout() {
    LayoutPreferences preferences(layoutFile_);
    preferences.reset();
    applyDefaultDockPlacement();
    studiesDock_->show();
    propertiesDock_->show();
    tasksDock_->show();
}

void WorkspaceWindow::showSample() {
    if (hasVolume_) {
        return;
    }
    QString error;
    if (!sampleViewport_->showSample(&error)) {
        actions_->reportError(error);
        errorBanner_->setMessage(Banner::Severity::Error, error);
        errorBanner_->setVisible(true);
        return;
    }
    actions_->showSample();
    errorBanner_->setVisible(false);
}

void WorkspaceWindow::closeEvent(QCloseEvent* event) {
    shutdownSurface();
    shutdownImport();
    saveLayout();
    QMainWindow::closeEvent(event);
}

void WorkspaceWindow::buildMenus() {
    auto* fileMenu = menuBar()->addMenu(QStringLiteral("&File"));
    importAction_ = fileMenu->addAction(QStringLiteral("&Import CT folder..."));
    importAction_->setObjectName(QStringLiteral("importCtAction"));
    importAction_->setShortcut(QKeySequence::Open);
    connect(importAction_, &QAction::triggered, this, [this] {
        const QString folder = QFileDialog::getExistingDirectory(
            this, QStringLiteral("Import CT folder"), QString(),
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (!folder.isEmpty()) {
            importFolder(folder);
        }
    });
    auto* saveMaskAction = fileMenu->addAction(QStringLiteral("Save &mask..."));
    saveMaskAction->setObjectName(QStringLiteral("saveMaskAction"));
    connect(saveMaskAction, &QAction::triggered, this, [this] {
        const QString path = QFileDialog::getSaveFileName(
            this, QStringLiteral("Save label mask"), QString(), QStringLiteral("MedForge mask (*.mfmask)"));
        if (path.isEmpty()) {
            return;
        }
        QString error;
        if (!saveMask(path, &error)) {
            errorBanner_->setMessage(Banner::Severity::Error, error);
            errorBanner_->setVisible(true);
        }
    });
    auto* loadMaskAction = fileMenu->addAction(QStringLiteral("&Load mask..."));
    loadMaskAction->setObjectName(QStringLiteral("loadMaskAction"));
    connect(loadMaskAction, &QAction::triggered, this, [this] {
        const QString path = QFileDialog::getOpenFileName(
            this, QStringLiteral("Load label mask"), QString(), QStringLiteral("MedForge mask (*.mfmask)"));
        if (path.isEmpty()) {
            return;
        }
        QString error;
        if (!loadMask(path, &error)) {
            errorBanner_->setMessage(Banner::Severity::Error, error);
            errorBanner_->setVisible(true);
        }
    });
    fileMenu->addSeparator();
    auto* quit = fileMenu->addAction(QStringLiteral("&Exit"));
    quit->setShortcut(QKeySequence::Quit);
    connect(quit, &QAction::triggered, this, &QWidget::close);

    auto* viewMenu = menuBar()->addMenu(QStringLiteral("&View"));
    showSampleAction_ = viewMenu->addAction(QStringLiteral("Show &sample geometry"));
    showSampleAction_->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));
    resetCameraAction_ = viewMenu->addAction(QStringLiteral("Reset &camera"));
    resetCameraAction_->setShortcut(QKeySequence(Qt::Key_Home));
    resetLayoutAction_ = viewMenu->addAction(QStringLiteral("Reset &layout"));
    connect(showSampleAction_, &QAction::triggered, this, &WorkspaceWindow::showSample);
    connect(resetCameraAction_, &QAction::triggered, this, [this] {
        if (hasVolume_) {
            QString error;
            if (!volumeViewport_->resetToBounds(&error)) {
                errorBanner_->setMessage(Banner::Severity::Error, error);
                errorBanner_->setVisible(true);
            }
            return;
        }
        if (!actions_->resetCamera()) {
            return;
        }
        QString error;
        if (!sampleViewport_->resetCamera(&error)) {
            actions_->reportError(error);
            errorBanner_->setMessage(Banner::Severity::Error, error);
            errorBanner_->setVisible(true);
        }
    });
    connect(resetLayoutAction_, &QAction::triggered, this, &WorkspaceWindow::resetLayout);

    auto* editMenu = menuBar()->addMenu(QStringLiteral("&Edit"));
    undoSegmentAction_ = editMenu->addAction(QStringLiteral("&Undo"));
    undoSegmentAction_->setObjectName(QStringLiteral("undoSegment"));
    undoSegmentAction_->setShortcut(QKeySequence::Undo);
    redoSegmentAction_ = editMenu->addAction(QStringLiteral("&Redo"));
    redoSegmentAction_->setObjectName(QStringLiteral("redoSegment"));
    redoSegmentAction_->setShortcut(QKeySequence::Redo);
    connect(undoSegmentAction_, &QAction::triggered, this, &WorkspaceWindow::undoSegment);
    connect(redoSegmentAction_, &QAction::triggered, this, &WorkspaceWindow::redoSegment);

    auto* helpMenu = menuBar()->addMenu(QStringLiteral("&Help"));
    auto* about = helpMenu->addAction(QStringLiteral("&About MedForge"));
    connect(about, &QAction::triggered, this, [this] {
        QMessageBox::information(this,
                                 QStringLiteral("About MedForge"),
                                 QStringLiteral("MedForge %1\n\n%2\n\nNot a medical device.")
                                     .arg(QString::fromUtf8(medforge::core::kVersionString),
                                          QString::fromUtf8(medforge::core::kResearchNotice)));
    });

    auto* toolbar = addToolBar(QStringLiteral("Main"));
    toolbar->setObjectName(QStringLiteral("mainToolbar"));
    toolbar->setMovable(false);
    toolbar->addAction(importAction_);
    toolbar->addAction(showSampleAction_);
    toolbar->addAction(resetCameraAction_);
    toolbar->addAction(resetLayoutAction_);
}

void WorkspaceWindow::buildDocks() {
    studiesDock_ = makeDock(QStringLiteral("Studies"), QStringLiteral("studiesDock"), this);
    auto* studies = new QWidget(studiesDock_);
    auto* studiesLayout = new QVBoxLayout(studies);
    studiesLayout->setContentsMargins(medforge::ui::tokens::space8,
                                      medforge::ui::tokens::space8,
                                      medforge::ui::tokens::space8,
                                      medforge::ui::tokens::space8);
    studiesRow_ = new BrowserRow(QStringLiteral("Studies"), studies);
    studiesRow_->setCountText(QStringLiteral("0"));
    seriesRow_ = new BrowserRow(QStringLiteral("CT series"), studies);
    seriesRow_->setDepth(1);
    seriesRow_->setObjectName(QStringLiteral("loadedSeriesRow"));
    seriesRow_->hide();
    surfaceRow_ = new BrowserRow(QStringLiteral("Surface"), studies);
    surfaceRow_->setDepth(1);
    surfaceRow_->setObjectName(QStringLiteral("surfaceRow"));
    surfaceRow_->hide();
    segmentRow_ = new BrowserRow(QStringLiteral("Segmentation"), studies);
    segmentRow_->setDepth(1);
    segmentRow_->setObjectName(QStringLiteral("segmentRow"));
    segmentRow_->hide();
    auto* sampleRow = new BrowserRow(QStringLiteral("Cube and sphere"), studies);
    sampleRow->setDepth(1);
    sampleRow->setObjectName(QStringLiteral("sampleSceneRow"));
    connect(sampleRow, &BrowserRow::activated, this, &WorkspaceWindow::showSample);
    studiesLayout->addWidget(studiesRow_);
    studiesLayout->addWidget(seriesRow_);
    studiesLayout->addWidget(surfaceRow_);
    studiesLayout->addWidget(segmentRow_);
    studiesLayout->addWidget(sampleRow);
    studiesLayout->addStretch(1);
    studiesDock_->setWidget(studies);

    propertiesDock_ =
        makeDock(QStringLiteral("Properties"), QStringLiteral("propertiesDock"), this);
    auto* properties = new QWidget(propertiesDock_);
    auto* propertiesLayout = new QVBoxLayout(properties);
    propertiesLayout->setContentsMargins(medforge::ui::tokens::space8,
                                         medforge::ui::tokens::space8,
                                         medforge::ui::tokens::space8,
                                         medforge::ui::tokens::space8);
    propertiesBody_ = new QLabel(properties);
    propertiesBody_->setObjectName(QStringLiteral("propertiesBody"));
    propertiesBody_->setWordWrap(true);
    ctProperties_ = new QWidget(properties);
    ctProperties_->setObjectName(QStringLiteral("ctProperties"));
    auto* ctLayout = new QVBoxLayout(ctProperties_);
    ctLayout->setContentsMargins(0, 0, 0, 0);
    ctLayout->setSpacing(medforge::ui::tokens::space8);
    windowPresets_ = new SegmentedControl(
        {QStringLiteral("Soft"),
         QStringLiteral("Lung"),
         QStringLiteral("Bone"),
         QStringLiteral("Brain")},
        ctProperties_);
    windowPresets_->setObjectName(QStringLiteral("windowLevelPresets"));
    windowPresets_->setToolTip(QStringLiteral("Display window presets. Not a clinical window."));
    windowWidth_ = new NumericEditor(ctProperties_);
    windowWidth_->setObjectName(QStringLiteral("windowWidth"));
    windowWidth_->setLabel(QStringLiteral("Window"));
    windowWidth_->setUnit(QStringLiteral("HU"));
    windowWidth_->setDecimals(0);
    windowWidth_->setRange(1, 8000);
    windowCenter_ = new NumericEditor(ctProperties_);
    windowCenter_->setObjectName(QStringLiteral("windowCenter"));
    windowCenter_->setLabel(QStringLiteral("Level"));
    windowCenter_->setUnit(QStringLiteral("HU"));
    windowCenter_->setDecimals(0);
    windowCenter_->setRange(-2000, 4000);
    auto* displayNote = new QLabel(
        QStringLiteral("Display presets only. Not a clinical window."), ctProperties_);
    displayNote->setWordWrap(true);
    displayNote->setProperty("mfRole", QStringLiteral("secondary"));
    ctLayout->addWidget(windowPresets_);
    ctLayout->addWidget(windowWidth_);
    ctLayout->addWidget(windowCenter_);
    ctLayout->addWidget(displayNote);
    volumePresets_ = new SegmentedControl(
        {QStringLiteral("Soft"), QStringLiteral("Lung"), QStringLiteral("Bone")}, ctProperties_);
    volumePresets_->setObjectName(QStringLiteral("volumePresets"));
    volumePresets_->setToolTip(QStringLiteral("Volume opacity presets. Not a clinical transfer function."));
    auto* visibility = new QWidget(ctProperties_);
    auto* visibilityLayout = new QHBoxLayout(visibility);
    visibilityLayout->setContentsMargins(0, 0, 0, 0);
    volumeVisibleButton_ = new QPushButton(QStringLiteral("Volume"), visibility);
    volumeVisibleButton_->setObjectName(QStringLiteral("volumeVisible"));
    volumeVisibleButton_->setCheckable(true);
    volumeVisibleButton_->setChecked(true);
    volumeVisibleButton_->setProperty("mfRole", QStringLiteral("segment"));
    meshVisibleButton_ = new QPushButton(QStringLiteral("Surface"), visibility);
    meshVisibleButton_->setObjectName(QStringLiteral("meshVisible"));
    meshVisibleButton_->setCheckable(true);
    meshVisibleButton_->setChecked(true);
    meshVisibleButton_->setProperty("mfRole", QStringLiteral("segment"));
    clipButton_ = new QPushButton(QStringLiteral("Clip"), visibility);
    clipButton_->setObjectName(QStringLiteral("clipPlane"));
    clipButton_->setCheckable(true);
    clipButton_->setProperty("mfRole", QStringLiteral("segment"));
    visibilityLayout->addWidget(volumeVisibleButton_);
    visibilityLayout->addWidget(meshVisibleButton_);
    visibilityLayout->addWidget(clipButton_);
    isoThreshold_ = new NumericEditor(ctProperties_);
    isoThreshold_->setObjectName(QStringLiteral("isoThreshold"));
    isoThreshold_->setLabel(QStringLiteral("Threshold"));
    isoThreshold_->setUnit(QStringLiteral("HU"));
    isoThreshold_->setDecimals(0);
    isoThreshold_->setRange(-1000, 3000);
    isoThreshold_->setValue(200);
    buildSurfaceButton_ = new QPushButton(QStringLiteral("Build surface"), ctProperties_);
    buildSurfaceButton_->setObjectName(QStringLiteral("buildSurface"));
    buildSurfaceButton_->setProperty("mfRole", QStringLiteral("primary"));
    cameraViews_ = new SegmentedControl(
        {QStringLiteral("A"), QStringLiteral("P"), QStringLiteral("L"), QStringLiteral("R"),
         QStringLiteral("S"), QStringLiteral("I")},
        ctProperties_);
    cameraViews_->setObjectName(QStringLiteral("cameraViews"));
    cameraViews_->setToolTip(QStringLiteral("Anatomical camera presets"));
    meshStats_ = new QLabel(QStringLiteral("No surface"), ctProperties_);
    meshStats_->setObjectName(QStringLiteral("meshStats"));
    meshStats_->setWordWrap(true);
    meshStats_->setProperty("mfRole", QStringLiteral("secondary"));
    auto* volumeNote = new QLabel(
        QStringLiteral("Research volume preview. Not a clinical reconstruction."), ctProperties_);
    volumeNote->setWordWrap(true);
    volumeNote->setProperty("mfRole", QStringLiteral("secondary"));
    auto* segmentNote = new QLabel(
        QStringLiteral("Research segmentation. Not a clinical contour."), ctProperties_);
    segmentNote->setWordWrap(true);
    segmentNote->setProperty("mfRole", QStringLiteral("secondary"));
    segmentTool_ = new SegmentedControl(
        {QStringLiteral("Threshold"), QStringLiteral("Grow"), QStringLiteral("Paint"),
         QStringLiteral("Erase")},
        ctProperties_);
    segmentTool_->setObjectName(QStringLiteral("segmentTool"));
    segmentTool_->setToolTip(QStringLiteral("Segmentation tool"));
    segmentLabel_ = new SegmentedControl({QStringLiteral("Label 1"), QStringLiteral("Label 2")},
                                         ctProperties_);
    segmentLabel_->setObjectName(QStringLiteral("segmentLabel"));
    segmentLabel_->setToolTip(QStringLiteral("Selected label"));
    segmentMin_ = new NumericEditor(ctProperties_);
    segmentMin_->setObjectName(QStringLiteral("segmentMin"));
    segmentMin_->setLabel(QStringLiteral("Min"));
    segmentMin_->setUnit(QStringLiteral("HU"));
    segmentMin_->setDecimals(0);
    segmentMin_->setRange(-2000, 4000);
    segmentMin_->setValue(200);
    segmentMax_ = new NumericEditor(ctProperties_);
    segmentMax_->setObjectName(QStringLiteral("segmentMax"));
    segmentMax_->setLabel(QStringLiteral("Max"));
    segmentMax_->setUnit(QStringLiteral("HU"));
    segmentMax_->setDecimals(0);
    segmentMax_->setRange(-2000, 4000);
    segmentMax_->setValue(1500);
    segmentRadius_ = new NumericEditor(ctProperties_);
    segmentRadius_->setObjectName(QStringLiteral("segmentRadius"));
    segmentRadius_->setLabel(QStringLiteral("Brush"));
    segmentRadius_->setUnit(QStringLiteral("mm"));
    segmentRadius_->setDecimals(1);
    segmentRadius_->setRange(0, 50);
    segmentRadius_->setValue(1);
    segmentConnectivity_ =
        new SegmentedControl({QStringLiteral("6-connected"), QStringLiteral("26-connected")},
                             ctProperties_);
    segmentConnectivity_->setObjectName(QStringLiteral("segmentConnectivity"));
    segmentConnectivity_->setToolTip(QStringLiteral("Region-grow connectivity"));
    segmentLimitSlice_ = new QCheckBox(QStringLiteral("Limit to axial slice"), ctProperties_);
    segmentLimitSlice_->setObjectName(QStringLiteral("segmentLimitSlice"));
    segmentPreviewToggle_ = new QCheckBox(QStringLiteral("Preview HU range"), ctProperties_);
    segmentPreviewToggle_->setObjectName(QStringLiteral("segmentPreviewToggle"));
    segmentPreview_ = new QLabel(QStringLiteral("0 voxels in range"), ctProperties_);
    segmentPreview_->setObjectName(QStringLiteral("segmentPreview"));
    segmentPreview_->setProperty("mfRole", QStringLiteral("secondary"));
    segmentUnsaved_ = new QLabel(QStringLiteral("Mask saved"), ctProperties_);
    segmentUnsaved_->setObjectName(QStringLiteral("segmentUnsaved"));
    segmentProvenance_ = new QLabel(ctProperties_);
    segmentProvenance_->setObjectName(QStringLiteral("segmentProvenance"));
    segmentProvenance_->setWordWrap(true);
    segmentProvenance_->setProperty("mfRole", QStringLiteral("secondary"));
    applyThresholdButton_ = new QPushButton(QStringLiteral("Apply threshold"), ctProperties_);
    applyThresholdButton_->setObjectName(QStringLiteral("applyThreshold"));
    regionGrowButton_ = new QPushButton(QStringLiteral("Grow from crosshair"), ctProperties_);
    regionGrowButton_->setObjectName(QStringLiteral("regionGrow"));
    labelMeshButton_ = new QPushButton(QStringLiteral("Mesh from label"), ctProperties_);
    labelMeshButton_->setObjectName(QStringLiteral("buildLabelMesh"));
    ctLayout->addWidget(volumePresets_);
    ctLayout->addWidget(visibility);
    ctLayout->addWidget(isoThreshold_);
    ctLayout->addWidget(buildSurfaceButton_);
    ctLayout->addWidget(cameraViews_);
    ctLayout->addWidget(meshStats_);
    ctLayout->addWidget(volumeNote);
    ctLayout->addWidget(segmentNote);
    ctLayout->addWidget(segmentTool_);
    ctLayout->addWidget(segmentLabel_);
    ctLayout->addWidget(segmentMin_);
    ctLayout->addWidget(segmentMax_);
    ctLayout->addWidget(segmentRadius_);
    ctLayout->addWidget(segmentConnectivity_);
    ctLayout->addWidget(segmentLimitSlice_);
    ctLayout->addWidget(segmentPreviewToggle_);
    ctLayout->addWidget(segmentPreview_);
    ctLayout->addWidget(applyThresholdButton_);
    ctLayout->addWidget(regionGrowButton_);
    ctLayout->addWidget(labelMeshButton_);
    ctLayout->addWidget(segmentUnsaved_);
    ctLayout->addWidget(segmentProvenance_);
    ctProperties_->hide();
    propertiesLayout->addWidget(propertiesBody_);
    propertiesLayout->addWidget(ctProperties_);
    propertiesLayout->addStretch(1);
    connect(windowPresets_, &SegmentedControl::currentIndexChanged, this, [this](int index) {
        medforge::imaging::WindowLevel preset = medforge::imaging::WindowLevel::softTissue();
        if (index == 1) {
            preset = medforge::imaging::WindowLevel::lung();
        } else if (index == 2) {
            preset = medforge::imaging::WindowLevel::bone();
        } else if (index == 3) {
            preset = medforge::imaging::WindowLevel::brain();
        }
        updatingWindowLevel_ = true;
        windowWidth_->setValue(preset.widthHu);
        windowCenter_->setValue(preset.centerHu);
        updatingWindowLevel_ = false;
        windowLevel_ = preset;
        refreshMpr();
    });
    connect(windowWidth_, &NumericEditor::valueChanged, this, [this](double value) {
        if (updatingWindowLevel_) {
            return;
        }
        windowLevel_.widthHu = value;
        refreshMpr();
    });
    connect(windowCenter_, &NumericEditor::valueChanged, this, [this](double value) {
        if (updatingWindowLevel_) {
            return;
        }
        windowLevel_.centerHu = value;
        refreshMpr();
    });
    connect(volumePresets_, &SegmentedControl::currentIndexChanged, this, [this](int index) {
        medforge::rendering::TransferFunction function = medforge::rendering::softTissueTransfer();
        if (index == 1) {
            function = medforge::rendering::lungTransfer();
        } else if (index == 2) {
            function = medforge::rendering::boneTransfer();
        }
        volumeViewport_->setTransferFunction(function);
    });
    connect(volumeVisibleButton_, &QPushButton::toggled, this, [this](bool checked) {
        volumeViewport_->setVolumeVisible(checked);
    });
    connect(meshVisibleButton_, &QPushButton::toggled, this, [this](bool checked) {
        if (!checked) {
            volumeViewport_->clearMesh();
            return;
        }
        refreshSurfaceView();
    });
    connect(clipButton_, &QPushButton::toggled, this, [this](bool) { refreshSurfaceView(); });
    connect(buildSurfaceButton_, &QPushButton::clicked, this, [this] { buildSurface(); });
    connect(cameraViews_, &SegmentedControl::currentIndexChanged, this, [this](int index) {
        const medforge::rendering::AnatomicalView views[] = {
            medforge::rendering::AnatomicalView::Anterior,
            medforge::rendering::AnatomicalView::Posterior,
            medforge::rendering::AnatomicalView::Left,
            medforge::rendering::AnatomicalView::Right,
            medforge::rendering::AnatomicalView::Superior,
            medforge::rendering::AnatomicalView::Inferior};
        const int clamped = std::clamp(index, 0, 5);
        volumeViewport_->applyAnatomicalView(views[clamped], nullptr);
    });
    connect(applyThresholdButton_, &QPushButton::clicked, this, &WorkspaceWindow::applySegmentThreshold);
    connect(regionGrowButton_, &QPushButton::clicked, this, [this] { growFromCrosshair(); });
    connect(labelMeshButton_, &QPushButton::clicked, this, [this] { buildLabelMesh(); });
    connect(segmentPreviewToggle_, &QCheckBox::toggled, this, [this](bool) { refreshMpr(); });
    const auto saveTools = [this](double) { saveSegmentTools(); };
    connect(segmentMin_, &NumericEditor::valueChanged, this, [this](double) {
        saveSegmentTools();
        refreshSegmentStatus();
        if (segmentPreviewToggle_->isChecked()) {
            refreshMpr();
        }
    });
    connect(segmentMax_, &NumericEditor::valueChanged, this, [this](double) {
        saveSegmentTools();
        refreshSegmentStatus();
        if (segmentPreviewToggle_->isChecked()) {
            refreshMpr();
        }
    });
    connect(segmentRadius_, &NumericEditor::valueChanged, this, saveTools);
    connect(segmentConnectivity_, &SegmentedControl::currentIndexChanged, this, [this](int) {
        saveSegmentTools();
    });
    connect(segmentLimitSlice_, &QCheckBox::toggled, this, [this](bool) { saveSegmentTools(); });
    connect(segmentTool_, &SegmentedControl::currentIndexChanged, this, [this](int) { saveSegmentTools(); });
    connect(segmentLabel_, &SegmentedControl::currentIndexChanged, this, [this](int) {
        saveSegmentTools();
        refreshSegmentStatus();
    });
    auto* propertiesScroll = new QScrollArea(propertiesDock_);
    propertiesScroll->setObjectName(QStringLiteral("propertiesScroll"));
    propertiesScroll->setWidgetResizable(true);
    propertiesScroll->setFrameShape(QFrame::NoFrame);
    propertiesScroll->setWidget(properties);
    propertiesDock_->setWidget(propertiesScroll);

    tasksDock_ = makeDock(QStringLiteral("Tasks"), QStringLiteral("tasksDock"), this);
    auto* tasks = new QWidget(tasksDock_);
    auto* tasksLayout = new QVBoxLayout(tasks);
    tasksLayout->setContentsMargins(medforge::ui::tokens::space8,
                                    medforge::ui::tokens::space8,
                                    medforge::ui::tokens::space8,
                                    medforge::ui::tokens::space8);
    taskIdleLabel_ =
        new QLabel(QStringLiteral(
                       "No background task. Import and surface extraction run off the GUI thread."),
                   tasks);
    taskIdleLabel_->setObjectName(QStringLiteral("taskStatus"));
    taskIdleLabel_->setWordWrap(true);
    taskIdleLabel_->setProperty("mfRole", QStringLiteral("secondary"));
    taskProgress_ = new TaskProgress(tasks);
    taskProgress_->setObjectName(QStringLiteral("importProgress"));
    tasksLayout->addWidget(taskIdleLabel_);
    tasksLayout->addWidget(taskProgress_);
    connect(taskProgress_, &TaskProgress::cancelled, this, [this] {
        if (segmentThread_.joinable()) {
            cancelSegment();
            return;
        }
        if (surfaceThread_.joinable()) {
            cancelSurface();
            return;
        }
        cancelImport();
    });
    tasksDock_->setWidget(tasks);

    auto* viewMenu = menuBar()->actions().at(1)->menu();
    viewMenu->addSeparator();
    viewMenu->addAction(studiesDock_->toggleViewAction());
    viewMenu->addAction(propertiesDock_->toggleViewAction());
    viewMenu->addAction(tasksDock_->toggleViewAction());
}

void WorkspaceWindow::buildViewports() {
    auto* central = new QWidget(this);
    auto* column = new QVBoxLayout(central);
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(0);
    researchLabel_ = new QLabel(QString::fromUtf8(medforge::core::kResearchNotice), central);
    researchLabel_->setObjectName(QStringLiteral("researchNotice"));
    researchLabel_->setMargin(medforge::ui::tokens::space8);
    errorBanner_ = new Banner(central);
    errorBanner_->setVisible(false);
    connect(errorBanner_, &Banner::dismissed, errorBanner_, &QWidget::hide);

    auto* gridHost = new QWidget(central);
    auto* grid = new QGridLayout(gridHost);
    grid->setContentsMargins(medforge::ui::tokens::space8,
                             medforge::ui::tokens::space8,
                             medforge::ui::tokens::space8,
                             medforge::ui::tokens::space8);
    grid->setSpacing(medforge::ui::tokens::space8);
    axialViewport_ = new MprViewport(QStringLiteral("Axial"), gridHost);
    axialViewport_->setObjectName(QStringLiteral("viewportAxial"));
    sagittalViewport_ = new MprViewport(QStringLiteral("Sagittal"), gridHost);
    sagittalViewport_->setObjectName(QStringLiteral("viewportSagittal"));
    coronalViewport_ = new MprViewport(QStringLiteral("Coronal"), gridHost);
    coronalViewport_->setObjectName(QStringLiteral("viewportCoronal"));
    grid->addWidget(axialViewport_, 0, 0);
    grid->addWidget(sagittalViewport_, 0, 1);
    grid->addWidget(coronalViewport_, 1, 0);
    grid->setRowStretch(0, 1);
    grid->setRowStretch(1, 1);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);
    const auto bindPlane = [this](MprViewport* view, medforge::imaging::MprPlane plane) {
        connect(view, &MprViewport::sliceScrolled, this, [this, plane](int steps) {
            if (!hasVolume_) {
                return;
            }
            medforge::imaging::scrollCrosshair(crosshair_, plane, steps, volume_);
            refreshMpr();
        });
        connect(view, &MprViewport::imageClicked, this, [this, plane](int x, int y) {
            handlePlanePointer(plane, x, y);
        });
        connect(view, &MprViewport::imageDragged, this, [this, plane](int x, int y) {
            handlePlanePointer(plane, x, y);
        });
    };
    bindPlane(axialViewport_, medforge::imaging::MprPlane::Axial);
    bindPlane(sagittalViewport_, medforge::imaging::MprPlane::Sagittal);
    bindPlane(coronalViewport_, medforge::imaging::MprPlane::Coronal);

    auto* view3d = new QFrame(gridHost);
    view3d->setProperty("mfRole", QStringLiteral("viewport"));
    auto* view3dLayout = new QVBoxLayout(view3d);
    view3dLayout->setContentsMargins(0, 0, 0, 0);
    view3dLayout->setSpacing(0);
    auto* view3dTitle = new QLabel(QStringLiteral("3D"), view3d);
    view3dTitle->setProperty("mfRole", QStringLiteral("section"));
    view3dTitle->setMargin(medforge::ui::tokens::space8);
    sampleViewport_ = new SampleViewport(view3d);
    volumeViewport_ = new VolumeViewport(view3d);
    volumeViewport_->hide();
    view3dLayout->addWidget(view3dTitle);
    view3dLayout->addWidget(sampleViewport_, 1);
    view3dLayout->addWidget(volumeViewport_, 1);
    grid->addWidget(view3d, 1, 1);

    column->addWidget(researchLabel_);
    column->addWidget(errorBanner_);
    column->addWidget(gridHost, 1);
    setCentralWidget(central);

    statusLabel_ = new QLabel(this);
    statusLabel_->setObjectName(QStringLiteral("workspaceStatus"));
    cursorLabel_ = new QLabel(this);
    cursorLabel_->setObjectName(QStringLiteral("cursorReadout"));
    statusBar()->addWidget(statusLabel_, 1);
    statusBar()->addPermanentWidget(cursorLabel_);
    statusBar()->addPermanentWidget(new QLabel(QStringLiteral("LPS mm"), this));
}

void WorkspaceWindow::applyDefaultDockPlacement() {
    addDockWidget(Qt::LeftDockWidgetArea, studiesDock_);
    addDockWidget(Qt::RightDockWidgetArea, propertiesDock_);
    addDockWidget(Qt::BottomDockWidgetArea, tasksDock_);
    studiesDock_->setMinimumWidth(180);
    propertiesDock_->setMinimumWidth(360);
    studiesDock_->show();
    propertiesDock_->show();
    tasksDock_->show();
}

void WorkspaceWindow::syncActions() {
    showSampleAction_->setEnabled(actions_->canShowSample());
    resetCameraAction_->setEnabled(actions_->canResetCamera());
    resetLayoutAction_->setEnabled(actions_->canResetLayout());
    importAction_->setEnabled(actions_->phase() != WorkspaceActions::Phase::Importing);
    statusLabel_->setText(actions_->statusText());
    if (actions_->phase() == WorkspaceActions::Phase::Error && !actions_->errorText().isEmpty()) {
        errorBanner_->setMessage(Banner::Severity::Error, actions_->errorText());
        errorBanner_->setVisible(true);
    }
}

void WorkspaceWindow::updateProperties() {
    if (hasVolume_) {
        propertiesBody_->setText(
            QStringLiteral("CT series\n%1\nResearch display only. Not patient data.")
                .arg(QString::fromStdString(volume_.seriesInstanceUid)));
        ctProperties_->setVisible(true);
        return;
    }
    ctProperties_->setVisible(false);
    if (actions_->phase() == WorkspaceActions::Phase::Ready) {
        propertiesBody_->setText(
            QStringLiteral("Sample\nCube and sphere\nDemo geometry. Not patient data."));
        return;
    }
    propertiesBody_->setText(QStringLiteral("No selection"));
}

void WorkspaceWindow::showVolumePane(bool visible) {
    sampleViewport_->setVisible(!visible);
    volumeViewport_->setVisible(visible);
}

void WorkspaceWindow::shutdownSurface() {
    ++surfaceGeneration_;
    surfaceCancel_.cancel();
    if (surfaceThread_.joinable()) {
        surfaceThread_.join();
    }
}

void WorkspaceWindow::cancelSurface() {
    surfaceCancel_.cancel();
}

void WorkspaceWindow::refreshSurfaceView() {
    medforge::core::Vec3 origin{};
    medforge::core::Vec3 normal{0, 0, 1};
    const bool clip = clipButton_->isChecked() && hasVolume_;
    if (clip) {
        const medforge::core::CoordinateResult lps =
            medforge::imaging::crosshairLps(crosshair_, volume_);
        if (lps.ok()) {
            origin = lps.value;
        }
        normal = {volume_.geometry.geometry.direction(0, 2),
                  volume_.geometry.geometry.direction(1, 2),
                  volume_.geometry.geometry.direction(2, 2)};
    }
    volumeViewport_->setClipPlane(clip, origin, normal);
    if (!hasSurface_) {
        meshStats_->setText(QStringLiteral("No surface"));
        surfaceRow_->hide();
        volumeViewport_->clearMesh();
        return;
    }
    const medforge::geometry::TriangleMesh shown =
        clip ? medforge::geometry::clipHalfSpace(surfaceMesh_, origin, normal) : surfaceMesh_;
    if (meshVisibleButton_->isChecked()) {
        volumeViewport_->setMesh(shown);
    } else {
        volumeViewport_->clearMesh();
    }
    const medforge::geometry::MeshStatistics stats = medforge::geometry::measureMesh(shown);
    meshStats_->setText(QStringLiteral("%1 triangles\nVolume %2 mm³\n%3 MB volume, %4 MB mesh\n%5 ms")
                            .arg(stats.triangleCount)
                            .arg(stats.volumeMm3, 0, 'f', 1)
                            .arg(static_cast<double>(surfaceResult_.volumeBytes) / 1.0e6, 0, 'f', 2)
                            .arg(static_cast<double>(surfaceResult_.meshBytes) / 1.0e6, 0, 'f', 2)
                            .arg(surfaceResult_.elapsedMs, 0, 'f', 0));
    surfaceRow_->setCountText(QString::number(stats.triangleCount));
    surfaceRow_->show();
}

void WorkspaceWindow::applySurface(int generation, const medforge::rendering::SurfaceResult& result) {
    if (surfaceThread_.joinable()) {
        surfaceThread_.join();
    }
    if (closing_ || generation != surfaceGeneration_) {
        return;
    }
    taskProgress_->setRunning(false);
    taskProgress_->setPercent(0);
    taskIdleLabel_->setText(
        QStringLiteral("No background task. Import and surface extraction run off the GUI thread."));
    if (result.status == medforge::rendering::SurfaceStatus::Cancelled) {
        errorBanner_->setMessage(Banner::Severity::Notice, QStringLiteral("Surface extraction cancelled"));
        errorBanner_->setVisible(true);
        return;
    }
    surfaceResult_ = result;
    if (result.status == medforge::rendering::SurfaceStatus::Empty) {
        hasSurface_ = false;
        surfaceMesh_ = {};
        refreshSurfaceView();
        errorBanner_->setMessage(Banner::Severity::Warning, QString::fromStdString(result.message));
        errorBanner_->setVisible(true);
        return;
    }
    if (result.status != medforge::rendering::SurfaceStatus::Ok) {
        errorBanner_->setMessage(Banner::Severity::Error, QString::fromStdString(result.message));
        errorBanner_->setVisible(true);
        return;
    }
    hasSurface_ = true;
    surfaceMesh_ = result.mesh;
    refreshSurfaceView();
    if (errorBanner_->isVisible() && errorBanner_->message().contains(QStringLiteral("surface"),
                                                                      Qt::CaseInsensitive)) {
        errorBanner_->hide();
    }
}

void WorkspaceWindow::buildSurface(int delayBeforeMs) {
    if (!hasVolume_ || actions_->phase() == WorkspaceActions::Phase::Importing ||
        surfaceThread_.joinable()) {
        return;
    }
    ++surfaceGeneration_;
    const int generation = surfaceGeneration_;
    surfaceCancel_ = medforge::core::CancellationSource();
    const medforge::core::CancellationToken token = surfaceCancel_.token();
    labelMeshRequest_ = false;
    const double threshold = isoThreshold_->value();
    medforge::geometry::ScalarGrid grid;
    grid.columns = volume_.columns;
    grid.rows = volume_.rows;
    grid.slices = volume_.slices;
    grid.values = volume_.hu;
    grid.geometry = volume_.geometry.geometry;
    taskIdleLabel_->setText(QStringLiteral("Building surface"));
    taskProgress_->setRunning(true);
    taskProgress_->setPercent(0);
    surfaceThread_ = std::thread([this, generation, grid = std::move(grid), threshold, token,
                                  delayBeforeMs] {
        const medforge::rendering::SurfaceResult result =
            medforge::rendering::extractIsosurface(grid, threshold, token, delayBeforeMs);
        QMetaObject::invokeMethod(
            this, [this, generation, result] { applySurface(generation, result); },
            Qt::QueuedConnection);
    });
}

void WorkspaceWindow::shutdownImport() {
    closing_ = true;
    importCancel_.cancel();
    if (importThread_.joinable()) {
        importThread_.join();
    }
}

void WorkspaceWindow::cancelImport() {
    if (actions_->phase() != WorkspaceActions::Phase::Importing) {
        return;
    }
    importCancel_.cancel();
}

void WorkspaceWindow::importFolder(const QString& folder, int delayPerFileMs) {
    if (actions_->phase() == WorkspaceActions::Phase::Importing || importThread_.joinable()) {
        return;
    }
    shutdownSegment();
    shutdownSurface();
    closing_ = false;
    importCancel_ = medforge::core::CancellationSource();
    const medforge::core::CancellationToken token = importCancel_.token();
    actions_->beginImport();
    errorBanner_->hide();
    taskIdleLabel_->setText(QStringLiteral("Importing CT series"));
    taskProgress_->setRunning(true);
    taskProgress_->setPercent(0);
    const std::filesystem::path path = folderPath(folder);
    importThread_ = std::thread([this, path, token, delayPerFileMs] {
        const medforge::imaging::ImportResult result = medforge::imaging::importCtFolder(
            medforge::imaging::ImportRequest{path, delayPerFileMs},
            token,
            [this](int done, int total) {
                QMetaObject::invokeMethod(
                    this,
                    [this, done, total] {
                        if (closing_ || total <= 0) {
                            return;
                        }
                        taskProgress_->setPercent(done * 100 / total);
                    },
                    Qt::QueuedConnection);
            });
        QMetaObject::invokeMethod(
            this, [this, result] { applyImportResult(result); }, Qt::QueuedConnection);
    });
}

void WorkspaceWindow::clearLoadedSeries() {
    hasVolume_ = false;
    volume_ = {};
    axialViewport_->clearImage();
    sagittalViewport_->clearImage();
    coronalViewport_->clearImage();
    cursorLabel_->clear();
    seriesRow_->hide();
    resetSegment();
    hasSurface_ = false;
    surfaceMesh_ = {};
    surfaceResult_ = {};
    meshStats_->setText(QStringLiteral("No surface"));
    surfaceRow_->hide();
    volumeViewport_->clearStudy();
    showVolumePane(false);
    studiesRow_->setCountText(QStringLiteral("0"));
}

void WorkspaceWindow::applyImportResult(const medforge::imaging::ImportResult& result) {
    if (importThread_.joinable()) {
        importThread_.join();
    }
    if (closing_) {
        return;
    }
    taskProgress_->setRunning(false);
    taskProgress_->setPercent(0);
    taskIdleLabel_->setText(
        QStringLiteral("No background task. Import and surface extraction run off the GUI thread."));
    if (result.status == medforge::imaging::ImportStatus::Cancelled) {
        if (hasVolume_) {
            actions_->showStudy(QStringLiteral("CT series  %1 slices").arg(volume_.slices));
        } else {
            actions_->noteCancelled();
        }
        errorBanner_->setMessage(Banner::Severity::Notice, QStringLiteral("Import cancelled"));
        errorBanner_->setVisible(true);
        return;
    }
    if (result.status != medforge::imaging::ImportStatus::Ok) {
        clearLoadedSeries();
        actions_->failImport(QString::fromStdString(result.message));
        updateProperties();
        return;
    }
    volume_ = result.build.volume;
    hasVolume_ = true;
    crosshair_.i = volume_.columns / 2;
    crosshair_.j = volume_.rows / 2;
    crosshair_.k = volume_.slices / 2;
    windowLevel_ = medforge::imaging::WindowLevel::softTissue();
    windowPresets_->blockSignals(true);
    windowPresets_->setCurrentIndex(0);
    windowPresets_->blockSignals(false);
    updatingWindowLevel_ = true;
    windowWidth_->setValue(windowLevel_.widthHu);
    windowCenter_->setValue(windowLevel_.centerHu);
    updatingWindowLevel_ = false;
    seriesRow_->setCountText(QString::number(volume_.slices));
    seriesRow_->show();
    hasSurface_ = false;
    surfaceMesh_ = {};
    surfaceResult_ = {};
    meshStats_->setText(QStringLiteral("No surface"));
    surfaceRow_->hide();
    labelMap_ = medforge::imaging::emptyLabelMap(volume_);
    segmentHistory_ = {};
    segmentRevision_ = 0;
    segmentSavedRevision_ = 0;
    refreshSegmentStatus();
    QString volumeError;
    if (!volumeViewport_->showVolume(volume_, &volumeError)) {
        errorBanner_->setMessage(Banner::Severity::Error, volumeError);
        errorBanner_->setVisible(true);
    } else {
        volumeViewport_->setTransferFunction(medforge::rendering::softTissueTransfer());
        showVolumePane(true);
    }
    studiesRow_->setCountText(QStringLiteral("1"));
    actions_->showStudy(QStringLiteral("CT series  %1 slices").arg(volume_.slices));
    if (result.message.empty()) {
        if (volumeError.isEmpty()) {
            errorBanner_->hide();
        }
    } else {
        errorBanner_->setMessage(Banner::Severity::Warning, QString::fromStdString(result.message));
        errorBanner_->setVisible(true);
    }
    refreshMpr();
}

void WorkspaceWindow::refreshMpr() {
    if (!hasVolume_) {
        return;
    }
    paintPlane(axialViewport_, medforge::imaging::MprPlane::Axial, crosshair_.k);
    paintPlane(sagittalViewport_, medforge::imaging::MprPlane::Sagittal, crosshair_.i);
    paintPlane(coronalViewport_, medforge::imaging::MprPlane::Coronal, crosshair_.j);
    refreshSegmentStatus();

    const medforge::core::CoordinateResult lps =
        medforge::imaging::crosshairLps(crosshair_, volume_);
    const float hu = volume_.huAt(crosshair_.i, crosshair_.j, crosshair_.k);
    const QString huText =
        std::isfinite(hu) ? QString::number(qRound(static_cast<double>(hu)))
                          : QStringLiteral("—");
    cursorLabel_->setText(QStringLiteral("i=%1  j=%2  k=%3    L %4  P %5  S %6 mm    HU %7")
                              .arg(crosshair_.i)
                              .arg(crosshair_.j)
                              .arg(crosshair_.k)
                              .arg(lps.value.x, 0, 'f', 2)
                              .arg(lps.value.y, 0, 'f', 2)
                              .arg(lps.value.z, 0, 'f', 2)
                              .arg(huText));
    if (clipButton_->isChecked()) {
        refreshSurfaceView();
    }
}

namespace {

QRgb tintGray(int gray, const QColor& color) {
    const int red = (gray * 3 + color.red()) / 4;
    const int green = (gray * 3 + color.green()) / 4;
    const int blue = (gray * 3 + color.blue()) / 4;
    return qRgb(red, green, blue);
}

medforge::geometry::ScalarGrid gridFromLabel(const medforge::imaging::LabelMap& map,
                                             std::uint8_t label) {
    medforge::geometry::ScalarGrid grid;
    grid.columns = map.columns;
    grid.rows = map.rows;
    grid.slices = map.slices;
    grid.geometry = map.geometry;
    grid.values.resize(map.voxels.size());
    for (std::size_t index = 0; index < map.voxels.size(); ++index) {
        grid.values[index] = map.voxels[index] == label ? 1.0F : 0.0F;
    }
    return grid;
}

} // namespace

void WorkspaceWindow::paintPlane(MprViewport* view, medforge::imaging::MprPlane plane, int index) {
    const medforge::imaging::MprImage slice = medforge::imaging::extractPlane(volume_, plane, index);
    int labelWidth = 0;
    int labelHeight = 0;
    std::vector<std::uint8_t> labels;
    if (labelMap_.columns == volume_.columns) {
        medforge::imaging::extractLabelPlane(labelMap_, plane, index, &labelWidth, &labelHeight, &labels);
    }
    const bool preview = segmentPreviewToggle_ != nullptr && segmentPreviewToggle_->isChecked();
    const float minHu = segmentMin_ != nullptr ? static_cast<float>(segmentMin_->value()) : 0;
    const float maxHu = segmentMax_ != nullptr ? static_cast<float>(segmentMax_->value()) : 0;
    bool colored = preview;
    for (std::uint8_t label : labels) {
        if (label != 0) {
            colored = true;
            break;
        }
    }
    QImage image(slice.width, slice.height, colored ? QImage::Format_RGB32 : QImage::Format_Grayscale8);
    for (int y = 0; y < slice.height; ++y) {
        for (int x = 0; x < slice.width; ++x) {
            const std::size_t offset = static_cast<std::size_t>(y) * static_cast<std::size_t>(slice.width) +
                                       static_cast<std::size_t>(x);
            const int gray = medforge::imaging::huToDisplayGray(slice.hu[offset], windowLevel_);
            if (!colored) {
                image.scanLine(y)[x] = static_cast<uchar>(gray);
                continue;
            }
            std::uint8_t label = offset < labels.size() ? labels[offset] : 0;
            QColor tint;
            if (label == 1) {
                tint = medforge::ui::tokens::accent;
            } else if (label == 2) {
                tint = medforge::ui::tokens::success;
            } else if (preview && slice.hu[offset] >= minHu && slice.hu[offset] <= maxHu) {
                tint = medforge::ui::tokens::warning;
            }
            reinterpret_cast<QRgb*>(image.scanLine(y))[x] =
                tint.isValid() ? tintGray(gray, tint) : qRgb(gray, gray, gray);
        }
    }
    view->setImage(image);
    const medforge::core::Vec3 spacing = volume_.geometry.geometry.spacingMm;
    if (plane == medforge::imaging::MprPlane::Axial) {
        view->setMillimetersPerPixel(spacing.x, spacing.y);
    } else if (plane == medforge::imaging::MprPlane::Sagittal) {
        view->setMillimetersPerPixel(spacing.y, spacing.z);
    } else {
        view->setMillimetersPerPixel(spacing.x, spacing.z);
    }
    const medforge::imaging::PlanePixel pixel = medforge::imaging::crosshairPixel(crosshair_, plane);
    view->setCrosshair(QPoint(pixel.x, pixel.y));
    const medforge::imaging::ViewLabels orientation = medforge::imaging::orientationLabels(volume_, plane);
    view->setOrientation(QString(QChar(orientation.left)), QString(QChar(orientation.right)),
                         QString(QChar(orientation.top)), QString(QChar(orientation.bottom)));
}

void WorkspaceWindow::handlePlanePointer(medforge::imaging::MprPlane plane, int x, int y) {
    if (!hasVolume_) {
        return;
    }
    medforge::imaging::moveCrosshairOnPlane(crosshair_, plane, x, y, volume_);
    paintSegment(plane, x, y);
    refreshMpr();
}

void WorkspaceWindow::resetSegment() {
    labelMap_ = {};
    segmentHistory_ = {};
    segmentRevision_ = 0;
    segmentSavedRevision_ = 0;
    labelMeshRequest_ = false;
    if (segmentRow_ != nullptr) {
        segmentRow_->hide();
    }
    if (segmentProvenance_ != nullptr) {
        segmentProvenance_->clear();
    }
    refreshSegmentStatus();
}

void WorkspaceWindow::commitSegmentEdit(medforge::imaging::LabelEdit edit, const char* name) {
    if (edit.changes.empty() || !hasVolume_) {
        return;
    }
    if (!segmentHistory_.execute(
            std::make_unique<medforge::imaging::LabelMapCommand>(&labelMap_, std::move(edit), name))) {
        return;
    }
    ++segmentRevision_;
    refreshMpr();
}

void WorkspaceWindow::refreshSegmentStatus() {
    if (undoSegmentAction_ != nullptr) {
        undoSegmentAction_->setEnabled(segmentHistory_.canUndo());
        redoSegmentAction_->setEnabled(segmentHistory_.canRedo());
    }
    if (segmentUnsaved_ == nullptr) {
        return;
    }
    if (!hasVolume_) {
        segmentUnsaved_->setText(QStringLiteral("Mask saved"));
        segmentPreview_->setText(QStringLiteral("0 voxels in range"));
        return;
    }
    segmentUnsaved_->setText(segmentUnsaved() ? QStringLiteral("Unsaved mask")
                                              : QStringLiteral("Mask saved"));
    const std::size_t previewCount = medforge::imaging::countInHuRange(
        volume_, static_cast<float>(segmentMin_->value()), static_cast<float>(segmentMax_->value()));
    segmentPreview_->setText(previewCount == 1
                                 ? QStringLiteral("1 voxel in range")
                                 : QStringLiteral("%1 voxels in range").arg(previewCount));
    const std::size_t labeled = medforge::imaging::countLabel(labelMap_, 1) +
                                medforge::imaging::countLabel(labelMap_, 2);
    segmentRow_->setCountText(QString::number(labeled));
    segmentRow_->setVisible(labeled > 0 || segmentUnsaved());
}

void WorkspaceWindow::saveSegmentTools() {
    if (segmentMin_ == nullptr) {
        return;
    }
    QFile file(layoutFile_ + QStringLiteral(".segmentation.ini"));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return;
    }
    QTextStream out(&file);
    out << "minHu=" << segmentMin_->value() << "\n";
    out << "maxHu=" << segmentMax_->value() << "\n";
    out << "radiusMm=" << segmentRadius_->value() << "\n";
    out << "tool=" << segmentTool_->currentIndex() << "\n";
    out << "label=" << segmentLabel_->currentIndex() << "\n";
    out << "connectivity=" << segmentConnectivity_->currentIndex() << "\n";
    out << "limitSlice=" << (segmentLimitSlice_->isChecked() ? 1 : 0) << "\n";
}

void WorkspaceWindow::loadSegmentTools() {
    QFile file(layoutFile_ + QStringLiteral(".segmentation.ini"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        refreshSegmentStatus();
        return;
    }
    double minHu = segmentMin_->value();
    double maxHu = segmentMax_->value();
    double radius = segmentRadius_->value();
    int tool = segmentTool_->currentIndex();
    int label = segmentLabel_->currentIndex();
    int connectivity = segmentConnectivity_->currentIndex();
    bool limitSlice = segmentLimitSlice_->isChecked();
    while (!file.atEnd()) {
        const QString line = QString::fromUtf8(file.readLine()).trimmed();
        const int split = line.indexOf(QLatin1Char('='));
        if (split <= 0) {
            continue;
        }
        const QString key = line.left(split);
        const QString value = line.mid(split + 1);
        if (key == QLatin1String("minHu")) {
            minHu = value.toDouble();
        } else if (key == QLatin1String("maxHu")) {
            maxHu = value.toDouble();
        } else if (key == QLatin1String("radiusMm")) {
            radius = value.toDouble();
        } else if (key == QLatin1String("tool")) {
            tool = value.toInt();
        } else if (key == QLatin1String("label")) {
            label = value.toInt();
        } else if (key == QLatin1String("connectivity")) {
            connectivity = value.toInt();
        } else if (key == QLatin1String("limitSlice")) {
            limitSlice = value.toInt() != 0;
        }
    }
    segmentMin_->setValue(minHu);
    segmentMax_->setValue(maxHu);
    segmentRadius_->setValue(radius);
    segmentTool_->setCurrentIndex(tool);
    segmentLabel_->setCurrentIndex(label);
    segmentConnectivity_->setCurrentIndex(connectivity);
    segmentLimitSlice_->setChecked(limitSlice);
    refreshSegmentStatus();
}

void WorkspaceWindow::shutdownSegment() {
    ++segmentGeneration_;
    segmentCancel_.cancel();
    if (segmentThread_.joinable()) {
        segmentThread_.join();
    }
}

void WorkspaceWindow::cancelSegment() {
    segmentCancel_.cancel();
}

void WorkspaceWindow::applySegmentThreshold() {
    if (!hasVolume_) {
        return;
    }
    if (labelMap_.columns != volume_.columns) {
        labelMap_ = medforge::imaging::emptyLabelMap(volume_);
    }
    const std::uint8_t label = static_cast<std::uint8_t>(segmentLabel_->currentIndex() + 1);
    commitSegmentEdit(medforge::imaging::thresholdEdit(volume_, labelMap_, label,
                                                       static_cast<float>(segmentMin_->value()),
                                                       static_cast<float>(segmentMax_->value())),
                      "threshold");
}

void WorkspaceWindow::growFromCrosshair(int delayBeforeMs) {
    if (!hasVolume_ || segmentThread_.joinable() || surfaceThread_.joinable()) {
        return;
    }
    if (labelMap_.columns != volume_.columns) {
        labelMap_ = medforge::imaging::emptyLabelMap(volume_);
    }
    ++segmentGeneration_;
    const int generation = segmentGeneration_;
    segmentCancel_ = medforge::core::CancellationSource();
    const medforge::core::CancellationToken token = segmentCancel_.token();
    medforge::imaging::IndexBounds bounds;
    if (segmentLimitSlice_->isChecked()) {
        bounds.limited = true;
        bounds.iMin = 0;
        bounds.iMax = volume_.columns - 1;
        bounds.jMin = 0;
        bounds.jMax = volume_.rows - 1;
        bounds.kMin = crosshair_.k;
        bounds.kMax = crosshair_.k;
    }
    const medforge::imaging::CtVolume volume = volume_;
    const medforge::imaging::LabelMap map = labelMap_;
    const int seedI = crosshair_.i;
    const int seedJ = crosshair_.j;
    const int seedK = crosshair_.k;
    const auto label = static_cast<std::uint8_t>(segmentLabel_->currentIndex() + 1);
    const float minHu = static_cast<float>(segmentMin_->value());
    const float maxHu = static_cast<float>(segmentMax_->value());
    const medforge::imaging::Connectivity connectivity =
        segmentConnectivity_->currentIndex() == 0 ? medforge::imaging::Connectivity::Faces6
                                                  : medforge::imaging::Connectivity::Neighborhood26;
    taskIdleLabel_->setText(QStringLiteral("Growing region"));
    taskProgress_->setRunning(true);
    taskProgress_->setPercent(0);
    segmentThread_ = std::thread([this, generation, volume, map, seedI, seedJ, seedK, label, minHu,
                                  maxHu, connectivity, bounds, token, delayBeforeMs] {
        const medforge::imaging::SegmentResult result = medforge::imaging::regionGrowEdit(
            volume, map, seedI, seedJ, seedK, label, minHu, maxHu, connectivity, bounds, token,
            delayBeforeMs);
        QMetaObject::invokeMethod(
            this, [this, generation, result] { applyGrow(generation, result); }, Qt::QueuedConnection);
    });
}

void WorkspaceWindow::applyGrow(int generation, const medforge::imaging::SegmentResult& result) {
    if (segmentThread_.joinable()) {
        segmentThread_.join();
    }
    if (closing_ || generation != segmentGeneration_) {
        return;
    }
    taskProgress_->setRunning(false);
    taskProgress_->setPercent(0);
    taskIdleLabel_->setText(
        QStringLiteral("No background task. Import and surface extraction run off the GUI thread."));
    if (result.cancelled) {
        errorBanner_->setMessage(Banner::Severity::Notice, QStringLiteral("Segmentation cancelled"));
        errorBanner_->setVisible(true);
        return;
    }
    if (result.edit.changes.empty()) {
        errorBanner_->setMessage(Banner::Severity::Warning,
                                 QString::fromStdString(result.message.empty() ? "Nothing to segment"
                                                                               : result.message));
        errorBanner_->setVisible(true);
        return;
    }
    commitSegmentEdit(result.edit, "grow");
    if (errorBanner_->isVisible() &&
        errorBanner_->message().contains(QStringLiteral("Seed"), Qt::CaseInsensitive)) {
        errorBanner_->hide();
    }
}

void WorkspaceWindow::paintSegment(medforge::imaging::MprPlane plane, int x, int y) {
    if (!hasVolume_ || segmentTool_ == nullptr) {
        return;
    }
    const int tool = segmentTool_->currentIndex();
    if (tool != 2 && tool != 3) {
        return;
    }
    if (labelMap_.columns != volume_.columns) {
        labelMap_ = medforge::imaging::emptyLabelMap(volume_);
    }
    medforge::imaging::BrushStamp stamp;
    stamp.plane = plane;
    stamp.x = x;
    stamp.y = y;
    stamp.radiusMm = segmentRadius_->value();
    stamp.label = tool == 3 ? 0 : static_cast<std::uint8_t>(segmentLabel_->currentIndex() + 1);
    if (plane == medforge::imaging::MprPlane::Axial) {
        stamp.slice = crosshair_.k;
    } else if (plane == medforge::imaging::MprPlane::Sagittal) {
        stamp.slice = crosshair_.i;
    } else {
        stamp.slice = crosshair_.j;
    }
    commitSegmentEdit(medforge::imaging::brushEdit(labelMap_, stamp), tool == 3 ? "erase" : "paint");
}

void WorkspaceWindow::undoSegment() {
    if (!segmentHistory_.undo()) {
        return;
    }
    --segmentRevision_;
    refreshMpr();
}

void WorkspaceWindow::redoSegment() {
    if (!segmentHistory_.redo()) {
        return;
    }
    ++segmentRevision_;
    refreshMpr();
}

bool WorkspaceWindow::saveMask(const QString& path, QString* error) {
    if (!hasVolume_) {
        if (error != nullptr) {
            *error = QStringLiteral("Import a CT series before saving a mask");
        }
        return false;
    }
    std::string message;
    if (!medforge::imaging::saveLabelMap(labelMap_, folderPath(path), &message)) {
        if (error != nullptr) {
            *error = QString::fromStdString(message);
        }
        return false;
    }
    segmentSavedRevision_ = segmentRevision_;
    refreshSegmentStatus();
    return true;
}

bool WorkspaceWindow::loadMask(const QString& path, QString* error) {
    if (!hasVolume_) {
        if (error != nullptr) {
            *error = QStringLiteral("Import a CT series before loading a mask");
        }
        return false;
    }
    medforge::imaging::LabelMap loaded;
    std::string message;
    if (!medforge::imaging::loadLabelMap(folderPath(path), &loaded, &message)) {
        if (error != nullptr) {
            *error = QString::fromStdString(message);
        }
        return false;
    }
    if (loaded.seriesInstanceUid != volume_.seriesInstanceUid || loaded.columns != volume_.columns ||
        loaded.rows != volume_.rows || loaded.slices != volume_.slices) {
        if (error != nullptr) {
            *error = QStringLiteral("Mask does not match this series");
        }
        return false;
    }
    segmentHistory_ = {};
    labelMap_ = std::move(loaded);
    segmentRevision_ = 0;
    segmentSavedRevision_ = 0;
    refreshMpr();
    return true;
}

void WorkspaceWindow::focusLabeledVoxel() {
    if (!hasVolume_ || segmentLabel_ == nullptr) {
        return;
    }
    const auto label = static_cast<std::uint8_t>(segmentLabel_->currentIndex() + 1);
    for (int k = 0; k < labelMap_.slices; ++k) {
        for (int j = 0; j < labelMap_.rows; ++j) {
            for (int i = 0; i < labelMap_.columns; ++i) {
                if (labelMap_.at(i, j, k) != label) {
                    continue;
                }
                crosshair_.i = i;
                crosshair_.j = j;
                crosshair_.k = k;
                refreshMpr();
                return;
            }
        }
    }
}

void WorkspaceWindow::buildLabelMesh(int delayBeforeMs) {
    if (!hasVolume_ || actions_->phase() == WorkspaceActions::Phase::Importing ||
        surfaceThread_.joinable() || segmentThread_.joinable()) {
        return;
    }
    const auto label = static_cast<std::uint8_t>(segmentLabel_->currentIndex() + 1);
    if (medforge::imaging::countLabel(labelMap_, label) == 0) {
        errorBanner_->setMessage(Banner::Severity::Warning, QStringLiteral("Label is empty"));
        errorBanner_->setVisible(true);
        return;
    }
    ++surfaceGeneration_;
    const int generation = surfaceGeneration_;
    surfaceCancel_ = medforge::core::CancellationSource();
    const medforge::core::CancellationToken token = surfaceCancel_.token();
    labelMeshRequest_ = true;
    medforge::geometry::ScalarGrid grid = gridFromLabel(labelMap_, label);
    taskIdleLabel_->setText(QStringLiteral("Building surface"));
    taskProgress_->setRunning(true);
    taskProgress_->setPercent(0);
    surfaceThread_ = std::thread([this, generation, grid = std::move(grid), token, delayBeforeMs, label] {
        const medforge::rendering::SurfaceResult result =
            medforge::rendering::extractIsosurface(grid, 0.5, token, delayBeforeMs);
        QMetaObject::invokeMethod(
            this,
            [this, generation, result, label] {
                applySurface(generation, result);
                if (result.status == medforge::rendering::SurfaceStatus::Ok && labelMeshRequest_) {
                    segmentProvenance_->setText(
                        QStringLiteral("Label %1 of %2")
                            .arg(label)
                            .arg(QString::fromStdString(volume_.seriesInstanceUid)));
                }
            },
            Qt::QueuedConnection);
    });
}

bool WorkspaceWindow::segmentUnsaved() const {
    return segmentRevision_ != segmentSavedRevision_;
}

int WorkspaceWindow::labelAt(int i, int j, int k) const {
    return labelMap_.at(i, j, k);
}
