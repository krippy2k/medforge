#include "medforge/ui/FoundationWindow.h"

#include "medforge/core/Version.h"
#include "medforge/ui/DesignTokens.h"
#include "medforge/ui/FoundationWidgets.h"
#include "medforge/ui/GalleryWindow.h"
#include "medforge/ui/WorkspaceReference.h"

#include <QAction>
#include <QDialog>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QMenuBar>
#include <QPushButton>
#include <QStatusBar>
#include <QVBoxLayout>

namespace {

void showAboutDialog(QWidget* parent) {
    QDialog dialog(parent);
    dialog.setWindowTitle(QStringLiteral("About MedForge"));
    dialog.setMinimumWidth(420);
    auto* column = new QVBoxLayout(&dialog);
    column->setContentsMargins(medforge::ui::tokens::space16,
                               medforge::ui::tokens::space16,
                               medforge::ui::tokens::space16,
                               medforge::ui::tokens::space16);
    column->setSpacing(medforge::ui::tokens::space8);
    auto* title = new QLabel(
        QStringLiteral("MedForge %1").arg(QString::fromUtf8(medforge::core::kVersionString)),
        &dialog);
    title->setProperty("mfRole", QStringLiteral("section"));
    auto* body =
        new QLabel(QStringLiteral("CT-to-CAD research workstation.\n\n%1\n\n"
                                  "Not a medical device. Do not use for diagnosis or treatment.")
                       .arg(QString::fromUtf8(medforge::core::kResearchNotice)),
                   &dialog);
    body->setWordWrap(true);
    auto* close = new QPushButton(QStringLiteral("Close"), &dialog);
    close->setProperty("mfRole", QStringLiteral("primary"));
    close->setDefault(true);
    QObject::connect(close, &QPushButton::clicked, &dialog, &QDialog::accept);
    column->addWidget(title);
    column->addWidget(body);
    column->addWidget(close, 0, Qt::AlignRight);
    dialog.exec();
}

} // namespace

FoundationWindow::FoundationWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("MedForge"));
    setMinimumSize(960, 640);
    resize(1100, 720);

    auto* fileMenu = menuBar()->addMenu(QStringLiteral("&File"));
    auto* quit = fileMenu->addAction(QStringLiteral("&Exit"));
    quit->setShortcut(QKeySequence::Quit);
    connect(quit, &QAction::triggered, this, &QWidget::close);

    auto* developerMenu = menuBar()->addMenu(QStringLiteral("&Developer"));
    auto* gallery = developerMenu->addAction(QStringLiteral("Component &gallery"));
    auto* reference = developerMenu->addAction(QStringLiteral("M1 layout &reference"));
    connect(gallery, &QAction::triggered, this, &FoundationWindow::openGallery);
    connect(reference, &QAction::triggered, this, &FoundationWindow::openReference);

    auto* helpMenu = menuBar()->addMenu(QStringLiteral("&Help"));
    auto* about = helpMenu->addAction(QStringLiteral("&About MedForge"));
    connect(about, &QAction::triggered, this, &FoundationWindow::showAbout);

    auto* central = new QWidget(this);
    auto* column = new QVBoxLayout(central);
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(0);
    auto* research = new Banner(central);
    research->setMessage(Banner::Severity::Notice,
                         QString::fromUtf8(medforge::core::kResearchNotice));
    research->setDismissible(false);
    column->addWidget(research);

    auto* body = new QWidget(central);
    auto* bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(medforge::ui::tokens::space24,
                                   medforge::ui::tokens::space24,
                                   medforge::ui::tokens::space24,
                                   medforge::ui::tokens::space24);
    bodyLayout->setSpacing(medforge::ui::tokens::space12);
    auto* title = new QLabel(QStringLiteral("MedForge foundation"), body);
    title->setProperty("mfRole", QStringLiteral("section"));
    auto* copy = new QLabel(
        QStringLiteral("This build establishes the application platform, design tokens, and "
                       "shared controls. The workstation shell, VTK viewport, and DICOM import "
                       "are later milestones."),
        body);
    copy->setWordWrap(true);
    copy->setMaximumWidth(720);
    auto* buttons = new QHBoxLayout();
    auto* openGallery = new QPushButton(QStringLiteral("Open component gallery"), body);
    openGallery->setProperty("mfRole", QStringLiteral("primary"));
    openGallery->setAccessibleName(QStringLiteral("Open component gallery"));
    auto* openReference = new QPushButton(QStringLiteral("Open layout reference"), body);
    openReference->setAccessibleName(QStringLiteral("Open layout reference"));
    buttons->addWidget(openGallery);
    buttons->addWidget(openReference);
    buttons->addStretch(1);
    connect(openGallery, &QPushButton::clicked, this, &FoundationWindow::openGallery);
    connect(openReference, &QPushButton::clicked, this, &FoundationWindow::openReference);
    bodyLayout->addWidget(title);
    bodyLayout->addWidget(copy);
    bodyLayout->addLayout(buttons);
    bodyLayout->addStretch(1);
    column->addWidget(body, 1);
    setCentralWidget(central);
    statusBar()->showMessage(QString::fromUtf8(medforge::core::kResearchNotice));
}

void FoundationWindow::openGallery() {
    if (gallery_.isNull()) {
        gallery_ = new GalleryWindow();
        gallery_->setAttribute(Qt::WA_DeleteOnClose);
    }
    gallery_->show();
    gallery_->raise();
    gallery_->activateWindow();
}

void FoundationWindow::openReference() {
    if (reference_.isNull()) {
        reference_ = new WorkspaceReference();
        reference_->setAttribute(Qt::WA_DeleteOnClose);
    }
    reference_->show();
    reference_->raise();
    reference_->activateWindow();
}

void FoundationWindow::showAbout() {
    showAboutDialog(this);
}
