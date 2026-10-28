#include "medforge/ui/WorkspaceWindow.h"

#include "medforge/core/Version.h"
#include "medforge/ui/DesignTokens.h"
#include "medforge/ui/FoundationWidgets.h"
#include "medforge/ui/LayoutPreferences.h"
#include "medforge/ui/SampleViewport.h"
#include "medforge/ui/WorkspaceActions.h"

#include <QAction>
#include <QCloseEvent>
#include <QDockWidget>
#include <QFrame>
#include <QGridLayout>
#include <QKeySequence>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>

namespace {

QDockWidget* makeDock(const QString& title, const QString& objectName, QWidget* parent) {
    auto* dock = new QDockWidget(title, parent);
    dock->setObjectName(objectName);
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea |
                          Qt::BottomDockWidgetArea);
    return dock;
}

QWidget* emptyViewport(const QString& title, const QString& objectName, QWidget* parent) {
    auto* frame = new QFrame(parent);
    frame->setObjectName(objectName);
    frame->setProperty("mfRole", QStringLiteral("viewport"));
    auto* layout = new QVBoxLayout(frame);
    layout->setContentsMargins(medforge::ui::tokens::space12,
                               medforge::ui::tokens::space12,
                               medforge::ui::tokens::space12,
                               medforge::ui::tokens::space12);
    auto* heading = new QLabel(title, frame);
    heading->setProperty("mfRole", QStringLiteral("section"));
    auto* body = new QLabel(QStringLiteral("No dataset loaded"), frame);
    body->setProperty("mfRole", QStringLiteral("secondary"));
    body->setWordWrap(true);
    layout->addWidget(heading);
    layout->addWidget(body);
    layout->addStretch(1);
    return frame;
}

} // namespace

WorkspaceWindow::WorkspaceWindow(const QString& layoutFile, QWidget* parent)
    : QMainWindow(parent), layoutFile_(layoutFile), actions_(new WorkspaceActions(this)) {
    setWindowTitle(QStringLiteral("MedForge"));
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

SampleViewport* WorkspaceWindow::sampleViewport() const {
    return sampleViewport_;
}

WorkspaceActions* WorkspaceWindow::actions() const {
    return actions_;
}

Banner* WorkspaceWindow::errorBanner() const {
    return errorBanner_;
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
    saveLayout();
    QMainWindow::closeEvent(event);
}

void WorkspaceWindow::buildMenus() {
    auto* fileMenu = menuBar()->addMenu(QStringLiteral("&File"));
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
    auto* studiesRow = new BrowserRow(QStringLiteral("Studies"), studies);
    studiesRow->setCountText(QStringLiteral("0"));
    auto* sampleRow = new BrowserRow(QStringLiteral("Cube and sphere"), studies);
    sampleRow->setDepth(1);
    sampleRow->setObjectName(QStringLiteral("sampleSceneRow"));
    connect(sampleRow, &BrowserRow::activated, this, &WorkspaceWindow::showSample);
    studiesLayout->addWidget(studiesRow);
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
    propertiesLayout->addWidget(propertiesBody_);
    propertiesLayout->addStretch(1);
    propertiesDock_->setWidget(properties);

    tasksDock_ = makeDock(QStringLiteral("Tasks"), QStringLiteral("tasksDock"), this);
    auto* tasks = new QWidget(tasksDock_);
    auto* tasksLayout = new QVBoxLayout(tasks);
    tasksLayout->setContentsMargins(medforge::ui::tokens::space8,
                                    medforge::ui::tokens::space8,
                                    medforge::ui::tokens::space8,
                                    medforge::ui::tokens::space8);
    auto* taskLabel =
        new QLabel(QStringLiteral("No background task. Rendering stays on the GUI thread."), tasks);
    taskLabel->setWordWrap(true);
    taskLabel->setProperty("mfRole", QStringLiteral("secondary"));
    tasksLayout->addWidget(taskLabel);
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
    grid->addWidget(
        emptyViewport(QStringLiteral("Axial"), QStringLiteral("viewportAxial"), gridHost), 0, 0);
    grid->addWidget(
        emptyViewport(QStringLiteral("Sagittal"), QStringLiteral("viewportSagittal"), gridHost),
        0,
        1);
    grid->addWidget(
        emptyViewport(QStringLiteral("Coronal"), QStringLiteral("viewportCoronal"), gridHost),
        1,
        0);

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
    statusBar()->addWidget(statusLabel_, 1);
    statusBar()->addPermanentWidget(new QLabel(QStringLiteral("LPS mm"), this));
}

void WorkspaceWindow::applyDefaultDockPlacement() {
    addDockWidget(Qt::LeftDockWidgetArea, studiesDock_);
    addDockWidget(Qt::RightDockWidgetArea, propertiesDock_);
    addDockWidget(Qt::BottomDockWidgetArea, tasksDock_);
    studiesDock_->show();
    propertiesDock_->show();
    tasksDock_->show();
}

void WorkspaceWindow::syncActions() {
    showSampleAction_->setEnabled(actions_->canShowSample());
    resetCameraAction_->setEnabled(actions_->canResetCamera());
    resetLayoutAction_->setEnabled(actions_->canResetLayout());
    statusLabel_->setText(actions_->statusText());
    if (actions_->phase() == WorkspaceActions::Phase::Error && !actions_->errorText().isEmpty()) {
        errorBanner_->setMessage(Banner::Severity::Error, actions_->errorText());
        errorBanner_->setVisible(true);
    }
}

void WorkspaceWindow::updateProperties() {
    if (actions_->phase() == WorkspaceActions::Phase::Ready) {
        propertiesBody_->setText(
            QStringLiteral("Sample\nCube and sphere\nDemo geometry. Not patient data."));
        return;
    }
    propertiesBody_->setText(QStringLiteral("No selection"));
}
