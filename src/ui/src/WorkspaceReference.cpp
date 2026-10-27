#include "medforge/ui/WorkspaceReference.h"

#include "medforge/core/Version.h"
#include "medforge/ui/DesignTokens.h"
#include "medforge/ui/FoundationWidgets.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace {

QFrame* region(const char* role, QWidget* parent) {
    auto* frame = new QFrame(parent);
    frame->setProperty("mfRole", QString::fromLatin1(role));
    return frame;
}

QLabel* annotation(const QString& text, QWidget* parent) {
    auto* label = new QLabel(text, parent);
    label->setProperty("mfRole", QStringLiteral("annotation"));
    label->setWordWrap(true);
    return label;
}

} // namespace

WorkspaceReference::WorkspaceReference(QWidget* parent) : QWidget(parent) {
    setWindowTitle(QStringLiteral("MedForge M1 workspace reference"));
    setMinimumSize(960, 640);
    resize(1280, 800);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* research = new Banner(this);
    research->setMessage(Banner::Severity::Notice,
                         QString::fromUtf8(medforge::core::kResearchNotice));
    research->setDismissible(false);
    root->addWidget(research);

    auto* caption = new QWidget(this);
    auto* captionLayout = new QVBoxLayout(caption);
    captionLayout->setContentsMargins(medforge::ui::tokens::space12,
                                      medforge::ui::tokens::space8,
                                      medforge::ui::tokens::space12,
                                      medforge::ui::tokens::space8);
    auto* title = new QLabel(QStringLiteral("M1 main window reference"), caption);
    title->setProperty("mfRole", QStringLiteral("section"));
    captionLayout->addWidget(title);
    captionLayout->addWidget(annotation(
        QStringLiteral("Annotated layout for the desktop shell. Panels are not functional docks "
                       "and there is no viewport yet."),
        caption));
    root->addWidget(caption);

    auto* toolbar = region("toolbar", this);
    auto* toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(medforge::ui::tokens::space12,
                                      medforge::ui::tokens::space8,
                                      medforge::ui::tokens::space12,
                                      medforge::ui::tokens::space8);
    auto* menus = new QLabel(QStringLiteral("File    Edit    View    Tools    Help"), toolbar);
    auto* slotNote =
        annotation(QStringLiteral("Toolbar slots: layout, reset view, units"), toolbar);
    toolbarLayout->addWidget(menus);
    toolbarLayout->addStretch(1);
    toolbarLayout->addWidget(slotNote);
    root->addWidget(toolbar);

    auto* body = new QHBoxLayout();
    body->setContentsMargins(medforge::ui::tokens::space8,
                             medforge::ui::tokens::space8,
                             medforge::ui::tokens::space8,
                             medforge::ui::tokens::space8);
    body->setSpacing(medforge::ui::tokens::space8);

    auto* studies = region("dock", this);
    studies->setMinimumWidth(220);
    studies->setMaximumWidth(300);
    auto* studiesLayout = new QVBoxLayout(studies);
    studiesLayout->setContentsMargins(0, 0, 0, medforge::ui::tokens::space8);
    studiesLayout->addWidget(new DockHeader(QStringLiteral("Studies"), studies));
    auto* study = new BrowserRow(QStringLiteral("Synthetic study"), studies);
    auto* series = new BrowserRow(QStringLiteral("Axial series"), studies);
    auto* volume = new BrowserRow(QStringLiteral("Volume"), studies);
    series->setDepth(1);
    volume->setDepth(2);
    study->setSelected(true);
    for (BrowserRow* row : {study, series, volume}) {
        row->setAttribute(Qt::WA_TransparentForMouseEvents);
        studiesLayout->addWidget(row);
    }
    studiesLayout->addWidget(annotation(QStringLiteral("Dataset browser"), studies));
    studiesLayout->addStretch(1);
    body->addWidget(studies, 2);

    auto* center = region("dock", this);
    auto* centerLayout = new QVBoxLayout(center);
    centerLayout->setContentsMargins(medforge::ui::tokens::space8,
                                     medforge::ui::tokens::space8,
                                     medforge::ui::tokens::space8,
                                     medforge::ui::tokens::space8);
    centerLayout->addWidget(annotation(QStringLiteral("Central viewports, 2×2"), center));
    auto* grid = new QGridLayout();
    grid->setSpacing(medforge::ui::tokens::space8);
    const QStringList planes = {QStringLiteral("Axial"),
                                QStringLiteral("Sagittal"),
                                QStringLiteral("Coronal"),
                                QStringLiteral("3D")};
    for (int index = 0; index < planes.size(); ++index) {
        auto* viewport = region("viewport", center);
        auto* viewportLayout = new QVBoxLayout(viewport);
        auto* plane = new QLabel(planes.at(index), viewport);
        plane->setProperty("mfRole", QStringLiteral("section"));
        plane->setAlignment(Qt::AlignCenter);
        viewportLayout->addStretch(1);
        viewportLayout->addWidget(plane);
        viewportLayout->addWidget(annotation(QStringLiteral("M1 viewport"), viewport));
        viewportLayout->addStretch(1);
        grid->addWidget(viewport, index / 2, index % 2);
    }
    centerLayout->addLayout(grid, 1);
    body->addWidget(center, 5);

    auto* inspector = region("dock", this);
    inspector->setMinimumWidth(240);
    inspector->setMaximumWidth(320);
    auto* inspectorLayout = new QVBoxLayout(inspector);
    inspectorLayout->setContentsMargins(medforge::ui::tokens::space8,
                                        0,
                                        medforge::ui::tokens::space8,
                                        medforge::ui::tokens::space8);
    inspectorLayout->addWidget(new DockHeader(QStringLiteral("Properties"), inspector));
    auto* spacing = new PropertyGroup(QStringLiteral("Volume"), inspector);
    auto* spacingNote =
        annotation(QStringLiteral("Spacing, origin, and LPS direction."), inspector);
    spacing->addRow(spacingNote);
    spacing->setAttribute(Qt::WA_TransparentForMouseEvents);
    inspectorLayout->addWidget(spacing);
    inspectorLayout->addWidget(annotation(QStringLiteral("Contextual inspector"), inspector));
    inspectorLayout->addStretch(1);
    body->addWidget(inspector, 2);
    root->addLayout(body, 1);

    auto* status = region("toolbar", this);
    auto* statusLayout = new QHBoxLayout(status);
    statusLayout->setContentsMargins(medforge::ui::tokens::space12,
                                     medforge::ui::tokens::space4,
                                     medforge::ui::tokens::space12,
                                     medforge::ui::tokens::space4);
    statusLayout->addWidget(new QLabel(QStringLiteral("Ready"), status));
    statusLayout->addStretch(1);
    statusLayout->addWidget(annotation(QStringLiteral("LPS millimeters"), status));
    statusLayout->addSpacing(medforge::ui::tokens::space16);
    auto* progress = new TaskProgress(status);
    progress->setPercent(0);
    progress->setRunning(false);
    progress->setAttribute(Qt::WA_TransparentForMouseEvents);
    progress->setMinimumWidth(220);
    statusLayout->addWidget(progress, 1);
    root->addWidget(status);
}
