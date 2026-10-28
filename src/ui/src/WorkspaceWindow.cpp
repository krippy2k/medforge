#include "medforge/ui/WorkspaceWindow.h"

#include "medforge/core/Version.h"
#include "medforge/imaging/CtVolume.h"
#include "medforge/ui/DesignTokens.h"
#include "medforge/ui/FoundationWidgets.h"
#include "medforge/ui/LayoutPreferences.h"
#include "medforge/ui/MprViewport.h"
#include "medforge/ui/NumericEditor.h"
#include "medforge/ui/SampleViewport.h"
#include "medforge/ui/SegmentedControl.h"
#include "medforge/ui/WorkspaceActions.h"

#include <QAction>
#include <QCloseEvent>
#include <QDockWidget>
#include <QFileDialog>
#include <QFrame>
#include <QGridLayout>
#include <QImage>
#include <QKeySequence>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
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
    connect(actions_, &WorkspaceActions::phaseChanged, this, [this] {
        syncActions();
        updateProperties();
    });
    syncActions();
    updateProperties();
    sampleViewport_->realizeNativeWindow();
}

WorkspaceWindow::~WorkspaceWindow() {
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
    auto* sampleRow = new BrowserRow(QStringLiteral("Cube and sphere"), studies);
    sampleRow->setDepth(1);
    sampleRow->setObjectName(QStringLiteral("sampleSceneRow"));
    connect(sampleRow, &BrowserRow::activated, this, &WorkspaceWindow::showSample);
    studiesLayout->addWidget(studiesRow_);
    studiesLayout->addWidget(seriesRow_);
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
    propertiesDock_->setWidget(properties);

    tasksDock_ = makeDock(QStringLiteral("Tasks"), QStringLiteral("tasksDock"), this);
    auto* tasks = new QWidget(tasksDock_);
    auto* tasksLayout = new QVBoxLayout(tasks);
    tasksLayout->setContentsMargins(medforge::ui::tokens::space8,
                                    medforge::ui::tokens::space8,
                                    medforge::ui::tokens::space8,
                                    medforge::ui::tokens::space8);
    taskIdleLabel_ =
        new QLabel(QStringLiteral("No background task. Import runs off the GUI thread."), tasks);
    taskIdleLabel_->setWordWrap(true);
    taskIdleLabel_->setProperty("mfRole", QStringLiteral("secondary"));
    taskProgress_ = new TaskProgress(tasks);
    taskProgress_->setObjectName(QStringLiteral("importProgress"));
    tasksLayout->addWidget(taskIdleLabel_);
    tasksLayout->addWidget(taskProgress_);
    connect(taskProgress_, &TaskProgress::cancelled, this, &WorkspaceWindow::cancelImport);
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
            if (!hasVolume_) {
                return;
            }
            medforge::imaging::moveCrosshairOnPlane(crosshair_, plane, x, y, volume_);
            refreshMpr();
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
    view3dLayout->addWidget(view3dTitle);
    view3dLayout->addWidget(sampleViewport_, 1);
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
    propertiesDock_->setMinimumWidth(240);
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
    taskIdleLabel_->setText(QStringLiteral("No background task. Import runs off the GUI thread."));
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
    studiesRow_->setCountText(QStringLiteral("1"));
    actions_->showStudy(QStringLiteral("CT series  %1 slices").arg(volume_.slices));
    if (result.message.empty()) {
        errorBanner_->hide();
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
    const auto paint = [this](MprViewport* view, medforge::imaging::MprPlane plane, int index) {
        const medforge::imaging::MprImage slice =
            medforge::imaging::extractPlane(volume_, plane, index);
        QImage image(slice.width, slice.height, QImage::Format_Grayscale8);
        for (int y = 0; y < slice.height; ++y) {
            uchar* line = image.scanLine(y);
            for (int x = 0; x < slice.width; ++x) {
                const std::size_t offset =
                    static_cast<std::size_t>(y) * static_cast<std::size_t>(slice.width) +
                    static_cast<std::size_t>(x);
                line[x] = medforge::imaging::huToDisplayGray(slice.hu[offset], windowLevel_);
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
        const medforge::imaging::PlanePixel pixel =
            medforge::imaging::crosshairPixel(crosshair_, plane);
        view->setCrosshair(QPoint(pixel.x, pixel.y));
        const medforge::imaging::ViewLabels labels =
            medforge::imaging::orientationLabels(volume_, plane);
        view->setOrientation(QString(QChar(labels.left)), QString(QChar(labels.right)),
                             QString(QChar(labels.top)), QString(QChar(labels.bottom)));
    };
    paint(axialViewport_, medforge::imaging::MprPlane::Axial, crosshair_.k);
    paint(sagittalViewport_, medforge::imaging::MprPlane::Sagittal, crosshair_.i);
    paint(coronalViewport_, medforge::imaging::MprPlane::Coronal, crosshair_.j);

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
}
