#include "medforge/ui/GalleryWindow.h"

#include "medforge/core/Version.h"
#include "medforge/ui/ConfirmDialog.h"
#include "medforge/ui/DesignTokens.h"
#include "medforge/ui/FoundationWidgets.h"
#include "medforge/ui/Icons.h"
#include "medforge/ui/NumericEditor.h"
#include "medforge/ui/SegmentedControl.h"

#include <QCloseEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMetaObject>
#include <QPointer>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QStatusBar>
#include <QThread>
#include <QThreadPool>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

class Section : public QWidget {
public:
    Section(const QString& title, QWidget* parent) : QWidget(parent) {
        setProperty("mfRole", QStringLiteral("sectionFrame"));
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(medforge::ui::tokens::space12,
                                   medforge::ui::tokens::space12,
                                   medforge::ui::tokens::space12,
                                   medforge::ui::tokens::space12);
        layout->setSpacing(medforge::ui::tokens::space8);
        auto* heading = new QLabel(title, this);
        heading->setProperty("mfRole", QStringLiteral("section"));
        layout->addWidget(heading);
        body = new QVBoxLayout();
        body->setSpacing(medforge::ui::tokens::space8);
        layout->addLayout(body);
        layout->addStretch(1);
    }

    QVBoxLayout* body = nullptr;
};

QPushButton* makeButton(const QString& text, const char* role) {
    auto* button = new QPushButton(text);
    button->setProperty("mfRole", QString::fromLatin1(role));
    button->setAccessibleName(text);
    button->setMinimumHeight(medforge::ui::tokens::controlHeight);
    return button;
}

} // namespace

GalleryWindow::GalleryWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("MedForge Component Gallery"));
    setMinimumSize(960, 640);
    resize(1280, 800);

    auto* central = new QWidget(this);
    auto* column = new QVBoxLayout(central);
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(0);

    auto* research = new Banner(central);
    research->setMessage(Banner::Severity::Notice,
                         QString::fromUtf8(medforge::core::kResearchNotice));
    research->setDismissible(false);
    column->addWidget(research);

    auto* scroll = new QScrollArea(central);
    scroll->setWidgetResizable(true);
    content_ = new QWidget(scroll);
    content_->setObjectName(QStringLiteral("galleryContent"));
    auto* contentLayout = new QVBoxLayout(content_);
    contentLayout->setContentsMargins(medforge::ui::tokens::space16,
                                      medforge::ui::tokens::space16,
                                      medforge::ui::tokens::space16,
                                      medforge::ui::tokens::space16);
    contentLayout->setSpacing(medforge::ui::tokens::space12);
    auto* intro = new QLabel(QStringLiteral("Shared workstation controls"), content_);
    intro->setProperty("mfRole", QStringLiteral("section"));
    auto* introNote = new QLabel(
        QStringLiteral("Developer gallery. These controls are the M1 building blocks."), content_);
    introNote->setProperty("mfRole", QStringLiteral("secondary"));
    introNote->setWordWrap(true);
    contentLayout->addWidget(intro);
    contentLayout->addWidget(introNote);

    grid_ = new QGridLayout();
    grid_->setHorizontalSpacing(medforge::ui::tokens::space16);
    grid_->setVerticalSpacing(medforge::ui::tokens::space16);
    contentLayout->addLayout(grid_);
    scroll->setWidget(content_);
    column->addWidget(scroll, 1);
    setCentralWidget(central);
    statusBar()->showMessage(QStringLiteral("Developer gallery"));

    auto* buttons = new Section(QStringLiteral("Buttons"), content_);
    auto* buttonRow = new QHBoxLayout();
    auto* apply = makeButton(QStringLiteral("Apply"), "primary");
    auto* reset = makeButton(QStringLiteral("Reset"), "secondary");
    auto* applyOff = makeButton(QStringLiteral("Apply"), "primary");
    auto* resetOff = makeButton(QStringLiteral("Reset"), "secondary");
    applyOff->setEnabled(false);
    resetOff->setEnabled(false);
    connect(apply, &QPushButton::clicked, this, [this]() {
        statusBar()->showMessage(QStringLiteral("Apply clicked"));
    });
    connect(reset, &QPushButton::clicked, this, [this]() {
        statusBar()->showMessage(QStringLiteral("Reset clicked"));
    });
    buttonRow->addWidget(apply);
    buttonRow->addWidget(reset);
    buttonRow->addWidget(applyOff);
    buttonRow->addWidget(resetOff);
    buttonRow->addStretch(1);
    buttons->body->addLayout(buttonRow);
    sections_.push_back(buttons);

    auto* tools = new Section(QStringLiteral("Tool buttons"), content_);
    auto* toolRow = new QHBoxLayout();
    const struct ToolSpec {
        QIcon icon;
        const char* name;
        bool enabled;
        bool checkable;
    } toolSpecs[] = {
        {medforge::ui::icons::search(), "Search", true, false},
        {medforge::ui::icons::folder(), "Studies", true, false},
        {medforge::ui::icons::check(), "Grid", true, true},
        {medforge::ui::icons::clear(), "Volume", false, false},
    };
    for (const ToolSpec& spec : toolSpecs) {
        auto* button = new QToolButton(tools);
        button->setIcon(spec.icon);
        button->setAccessibleName(QString::fromUtf8(spec.name));
        button->setToolTip(QString::fromUtf8(spec.name));
        button->setEnabled(spec.enabled);
        button->setCheckable(spec.checkable);
        if (spec.checkable) {
            button->setChecked(true);
        }
        if (spec.enabled) {
            const QString name = QString::fromUtf8(spec.name);
            connect(button, &QToolButton::clicked, this, [this, name]() {
                statusBar()->showMessage(name + QStringLiteral(" clicked"));
            });
        }
        toolRow->addWidget(button);
    }
    toolRow->addStretch(1);
    tools->body->addLayout(toolRow);
    sections_.push_back(tools);

    auto* segments = new Section(QStringLiteral("Segmented control"), content_);
    auto* planes = new SegmentedControl(
        {QStringLiteral("Axial"), QStringLiteral("Sagittal"), QStringLiteral("Coronal")}, segments);
    auto* planesOff = new SegmentedControl(
        {QStringLiteral("Axial"), QStringLiteral("Sagittal"), QStringLiteral("Coronal")}, segments);
    planesOff->setEnabled(false);
    connect(planes, &SegmentedControl::currentIndexChanged, this, [this, planes](int) {
        statusBar()->showMessage(planes->currentIndex() == 0   ? QStringLiteral("Axial")
                                 : planes->currentIndex() == 1 ? QStringLiteral("Sagittal")
                                                               : QStringLiteral("Coronal"));
    });
    segments->body->addWidget(planes);
    segments->body->addWidget(planesOff);
    sections_.push_back(segments);

    auto* numbers = new Section(QStringLiteral("Numeric editor"), content_);
    auto* length = new NumericEditor(numbers);
    length->setLabel(QStringLiteral("Length"));
    length->setUnit(QStringLiteral("mm"));
    length->setRange(0, 500);
    length->setDecimals(1);
    length->setValue(12.5);
    auto* spacing = new NumericEditor(numbers);
    spacing->setLabel(QStringLiteral("Spacing"));
    spacing->setUnit(QStringLiteral("mm"));
    spacing->setRange(0.1, 5);
    spacing->setDecimals(2);
    auto* spacingEdit = spacing->findChild<QLineEdit*>(QStringLiteral("numericValue"));
    spacingEdit->setText(QStringLiteral("0"));
    spacing->commitText();
    auto* lengthOff = new NumericEditor(numbers);
    lengthOff->setLabel(QStringLiteral("Threshold"));
    lengthOff->setUnit(QStringLiteral("HU"));
    lengthOff->setRange(-1000, 3000);
    lengthOff->setValue(400);
    lengthOff->setEnabled(false);
    numbers->body->addWidget(length);
    numbers->body->addWidget(spacing);
    numbers->body->addWidget(lengthOff);
    sections_.push_back(numbers);

    auto* browser = new Section(QStringLiteral("Search and browser"), content_);
    auto* search = new SearchField(browser);
    QList<BrowserRow*> rows;
    const struct RowSpec {
        const char* label;
        int depth;
        const char* count;
    } rowSpecs[] = {
        {"Synthetic study", 0, "1"},
        {"Axial series", 1, "1"},
        {"Volume", 2, "8x8x8"},
    };
    for (const RowSpec& spec : rowSpecs) {
        auto* row = new BrowserRow(QString::fromUtf8(spec.label), browser);
        row->setDepth(spec.depth);
        row->setCountText(QString::fromUtf8(spec.count));
        rows.push_back(row);
    }
    rows.first()->setSelected(true);
    connect(search, &QLineEdit::textChanged, this, [rows](const QString& text) {
        for (BrowserRow* row : rows) {
            row->setVisible(text.isEmpty() || row->label().contains(text, Qt::CaseInsensitive));
        }
    });
    for (BrowserRow* row : rows) {
        connect(row, &BrowserRow::activated, this, [this, rows, row]() {
            for (BrowserRow* other : rows) {
                other->setSelected(other == row);
            }
            statusBar()->showMessage(row->label() + QStringLiteral(" selected"));
        });
    }
    browser->body->addWidget(search);
    for (BrowserRow* row : rows) {
        browser->body->addWidget(row);
    }
    sections_.push_back(browser);

    auto* properties = new Section(QStringLiteral("Property group"), content_);
    auto* group = new PropertyGroup(QStringLiteral("Spacing"), properties);
    auto* rowSpacing = new NumericEditor(group);
    rowSpacing->setLabel(QStringLiteral("X"));
    rowSpacing->setUnit(QStringLiteral("mm"));
    rowSpacing->setRange(0.1, 10);
    rowSpacing->setDecimals(2);
    rowSpacing->setValue(0.5);
    group->addRow(rowSpacing);
    properties->body->addWidget(group);
    sections_.push_back(properties);

    auto* dock = new Section(QStringLiteral("Dock header"), content_);
    auto* header = new DockHeader(QStringLiteral("Properties"), dock);
    auto* collapse = header->addToolButton(medforge::ui::icons::chevronDown(),
                                           QStringLiteral("Collapse properties"));
    connect(collapse, &QToolButton::clicked, this, [group, collapse]() {
        const bool expanded = !group->isExpanded();
        group->setExpanded(expanded);
        collapse->setIcon(expanded ? medforge::ui::icons::chevronDown()
                                   : medforge::ui::icons::chevronRight());
    });
    dock->body->addWidget(header);
    sections_.push_back(dock);

    auto* task = new Section(QStringLiteral("Progress"), content_);
    taskProgress_ = new TaskProgress(task);
    taskProgress_->setPercent(42);
    taskProgress_->setRunning(true);
    auto* run = makeButton(QStringLiteral("Run sample task"), "primary");
    connect(run, &QPushButton::clicked, this, &GalleryWindow::startSampleTask);
    connect(taskProgress_, &TaskProgress::cancelled, this, &GalleryWindow::cancelSampleTask);
    task->body->addWidget(taskProgress_);
    task->body->addWidget(run, 0, Qt::AlignLeft);
    sections_.push_back(task);

    auto* notices = new Section(QStringLiteral("Notifications"), content_);
    const struct NoticeSpec {
        Banner::Severity severity;
        const char* message;
    } noticesSpec[] = {
        {Banner::Severity::Notice, "Layout restored."},
        {Banner::Severity::Warning, "Spacing is anisotropic."},
        {Banner::Severity::Error, "This sample file could not be read."},
        {Banner::Severity::Success, "Synthetic fixture written."},
    };
    for (const NoticeSpec& spec : noticesSpec) {
        auto* banner = new Banner(notices);
        banner->setMessage(spec.severity, QString::fromUtf8(spec.message));
        connect(banner, &Banner::dismissed, this, [this, banner]() {
            statusBar()->showMessage(banner->severityText() + QStringLiteral(" dismissed"));
            banner->hide();
        });
        notices->body->addWidget(banner);
    }
    sections_.push_back(notices);

    auto* emptySection = new Section(QStringLiteral("Empty state"), content_);
    auto* empty = new EmptyState(QStringLiteral("No measurement"),
                                 QStringLiteral("Landmark tools arrive in a later milestone. "
                                                "This action only reports that it was used."),
                                 QStringLiteral("Sample action"),
                                 emptySection);
    connect(empty, &EmptyState::actionTriggered, this, [this]() {
        statusBar()->showMessage(QStringLiteral("Sample action invoked"));
    });
    emptySection->body->addWidget(empty);
    sections_.push_back(emptySection);

    auto* validation = new Section(QStringLiteral("Validation"), content_);
    auto* message = new ValidationMessage(validation);
    message->setError(QStringLiteral("Spacing"), QStringLiteral("Value must be positive."));
    validation->body->addWidget(message);
    sections_.push_back(validation);

    auto* dialogSection = new Section(QStringLiteral("Confirmation dialog"), content_);
    auto* showDialog = makeButton(QStringLiteral("Delete landmark…"), "destructive");
    connect(showDialog, &QPushButton::clicked, this, [this]() {
        const bool accepted = ConfirmDialog::ask(
            this,
            QStringLiteral("Delete landmark"),
            QStringLiteral("Delete the selected landmark from this research document? "
                           "This sample does not delete data."),
            QStringLiteral("Delete"));
        statusBar()->showMessage(accepted ? QStringLiteral("Delete confirmed")
                                          : QStringLiteral("Delete cancelled"));
    });
    dialogSection->body->addWidget(showDialog, 0, Qt::AlignLeft);
    sections_.push_back(dialogSection);

    relayout();
}

void GalleryWindow::relayout() {
    if (grid_ == nullptr || sections_.isEmpty()) {
        return;
    }
    const int columns = width() >= 2200 ? 3 : (width() >= 1000 ? 2 : 1);
    if (columns == columns_) {
        return;
    }
    columns_ = columns;
    while (grid_->count() > 0) {
        grid_->takeAt(0);
    }
    for (int index = 0; index < static_cast<int>(sections_.size()); ++index) {
        grid_->addWidget(sections_.at(index), index / columns, index % columns);
    }
    for (int column = 0; column < columns; ++column) {
        grid_->setColumnStretch(column, 1);
    }
}

void GalleryWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    relayout();
}

void GalleryWindow::closeEvent(QCloseEvent* event) {
    taskSource_.cancel();
    QMainWindow::closeEvent(event);
}

void GalleryWindow::startSampleTask() {
    if (taskRunning_) {
        return;
    }
    taskSource_ = medforge::core::CancellationSource();
    taskRunning_ = true;
    taskProgress_->setRunning(true);
    taskProgress_->setPercent(0);
    statusBar()->showMessage(QStringLiteral("Sample task running"));
    const QPointer<GalleryWindow> window(this);
    const medforge::core::CancellationToken token = taskSource_.token();
    QThreadPool::globalInstance()->start([window, token]() {
        for (int step = 1; step <= 100; ++step) {
            if (token.isCancelled() || window.isNull()) {
                if (window) {
                    QMetaObject::invokeMethod(
                        window,
                        [window]() {
                            if (window) {
                                window->finishSampleTask(true);
                            }
                        },
                        Qt::QueuedConnection);
                }
                return;
            }
            QThread::msleep(20);
            const int percent = step;
            if (window) {
                QMetaObject::invokeMethod(
                    window,
                    [window, percent]() {
                        if (window) {
                            window->setSampleProgress(percent);
                        }
                    },
                    Qt::QueuedConnection);
            }
        }
        if (window) {
            QMetaObject::invokeMethod(
                window,
                [window]() {
                    if (window) {
                        window->finishSampleTask(false);
                    }
                },
                Qt::QueuedConnection);
        }
    });
}

void GalleryWindow::cancelSampleTask() {
    if (taskRunning_) {
        taskSource_.cancel();
        statusBar()->showMessage(QStringLiteral("Cancelling sample task"));
        return;
    }
    taskProgress_->setRunning(false);
    statusBar()->showMessage(QStringLiteral("Sample task cancelled"));
}

void GalleryWindow::setSampleProgress(int percent) {
    if (!taskRunning_ || taskProgress_ == nullptr) {
        return;
    }
    taskProgress_->setPercent(percent);
}

void GalleryWindow::finishSampleTask(bool cancelled) {
    if (!taskRunning_) {
        return;
    }
    taskRunning_ = false;
    if (taskProgress_ != nullptr) {
        taskProgress_->setRunning(false);
    }
    statusBar()->showMessage(cancelled ? QStringLiteral("Sample task cancelled")
                                       : QStringLiteral("Sample task finished"));
}
