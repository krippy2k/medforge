#include "medforge/ui/FoundationWidgets.h"

#include "medforge/ui/DesignTokens.h"
#include "medforge/ui/Icons.h"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QProgressBar>
#include <QPushButton>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>
#include <algorithm>

namespace {

void repolish(QWidget* widget) {
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

QString severityKey(Banner::Severity severity) {
    switch (severity) {
    case Banner::Severity::Warning:
        return QStringLiteral("warning");
    case Banner::Severity::Error:
        return QStringLiteral("error");
    case Banner::Severity::Success:
        return QStringLiteral("success");
    case Banner::Severity::Notice:
        break;
    }
    return QStringLiteral("notice");
}

} // namespace

SearchField::SearchField(QWidget* parent) : QLineEdit(parent) {
    setPlaceholderText(QStringLiteral("Search"));
    setAccessibleName(QStringLiteral("Search"));
    setClearButtonEnabled(true);
}

BrowserRow::BrowserRow(const QString& label, QWidget* parent) : QWidget(parent), label_(label) {
    setProperty("mfRole", QStringLiteral("browserRow"));
    setFocusPolicy(Qt::StrongFocus);
    setAccessibleName(label);
    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(medforge::ui::tokens::space4,
                            medforge::ui::tokens::space4,
                            medforge::ui::tokens::space8,
                            medforge::ui::tokens::space4);
    row->setSpacing(medforge::ui::tokens::space4);
    indent_ = new QWidget(this);
    indent_->setStyleSheet(QStringLiteral("background: transparent; border: none;"));
    indent_->setFixedWidth(0);
    toggle_ = new QToolButton(this);
    toggle_->setProperty("mfRole", QStringLiteral("inline"));
    toggle_->setIcon(medforge::ui::icons::chevronDown());
    toggle_->setAutoRaise(true);
    toggle_->setAccessibleName(QStringLiteral("Expand ") + label);
    toggle_->setFocusPolicy(Qt::NoFocus);
    labelWidget_ = new QLabel(label, this);
    labelWidget_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    countWidget_ = new QLabel(this);
    countWidget_->setProperty("mfRole", QStringLiteral("secondary"));
    countWidget_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    row->addWidget(indent_);
    row->addWidget(toggle_);
    row->addWidget(labelWidget_);
    row->addStretch(1);
    row->addWidget(countWidget_);
    setMinimumHeight(medforge::ui::tokens::controlHeight);
    connect(toggle_, &QToolButton::clicked, this, [this]() { setExpanded(!expanded_); });
}

void BrowserRow::setDepth(int depth) {
    indent_->setFixedWidth(std::max(0, depth) * medforge::ui::tokens::space16);
}

void BrowserRow::setCountText(const QString& text) {
    countWidget_->setText(text);
    countWidget_->setMinimumWidth(countWidget_->fontMetrics().horizontalAdvance(text) +
                                  medforge::ui::tokens::space8);
}

void BrowserRow::setSelected(bool selected) {
    selected_ = selected;
    setProperty("mfSelected", selected);
    repolish(this);
}

void BrowserRow::setExpanded(bool expanded) {
    expanded_ = expanded;
    toggle_->setIcon(expanded ? medforge::ui::icons::chevronDown()
                              : medforge::ui::icons::chevronRight());
}

bool BrowserRow::isSelected() const {
    return selected_;
}

bool BrowserRow::isExpanded() const {
    return expanded_;
}

QString BrowserRow::label() const {
    return label_;
}

void BrowserRow::mousePressEvent(QMouseEvent* event) {
    setSelected(true);
    emit activated();
    QWidget::mousePressEvent(event);
}

void BrowserRow::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Space) {
        setSelected(true);
        emit activated();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Right) {
        setExpanded(true);
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Left) {
        setExpanded(false);
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

PropertyGroup::PropertyGroup(const QString& title, QWidget* parent) : QWidget(parent) {
    auto* column = new QVBoxLayout(this);
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(medforge::ui::tokens::space8);
    toggle_ = new QToolButton(this);
    toggle_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toggle_->setText(title);
    toggle_->setIcon(medforge::ui::icons::chevronDown());
    toggle_->setAutoRaise(true);
    toggle_->setAccessibleName(title);
    toggle_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    toggle_->setMinimumHeight(medforge::ui::tokens::controlHeight);
    content_ = new QWidget(this);
    contentLayout_ = new QVBoxLayout(content_);
    contentLayout_->setContentsMargins(medforge::ui::tokens::space8, 0, 0, 0);
    contentLayout_->setSpacing(medforge::ui::tokens::space8);
    column->addWidget(toggle_);
    column->addWidget(content_);
    connect(toggle_, &QToolButton::clicked, this, [this]() { setExpanded(!expanded_); });
}

void PropertyGroup::addRow(QWidget* row) {
    contentLayout_->addWidget(row);
}

void PropertyGroup::setExpanded(bool expanded) {
    expanded_ = expanded;
    content_->setVisible(expanded);
    toggle_->setIcon(expanded ? medforge::ui::icons::chevronDown()
                              : medforge::ui::icons::chevronRight());
}

bool PropertyGroup::isExpanded() const {
    return expanded_;
}

DockHeader::DockHeader(const QString& title, QWidget* parent) : QWidget(parent) {
    setProperty("mfRole", QStringLiteral("dockHeader"));
    setMinimumHeight(medforge::ui::tokens::controlHeight);
    layout_ = new QHBoxLayout(this);
    layout_->setContentsMargins(medforge::ui::tokens::space8, 0, medforge::ui::tokens::space4, 0);
    auto* label = new QLabel(title, this);
    label->setProperty("mfRole", QStringLiteral("section"));
    layout_->addWidget(label);
    layout_->addStretch(1);
}

QToolButton* DockHeader::addToolButton(const QIcon& icon, const QString& accessibleName) {
    auto* button = new QToolButton(this);
    button->setProperty("mfRole", QStringLiteral("inline"));
    button->setIcon(icon);
    button->setAutoRaise(true);
    button->setAccessibleName(accessibleName);
    button->setToolTip(accessibleName);
    layout_->addWidget(button);
    return button;
}

TaskProgress::TaskProgress(QWidget* parent) : QWidget(parent) {
    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(medforge::ui::tokens::space8);
    bar_ = new QProgressBar(this);
    bar_->setRange(0, 100);
    bar_->setValue(0);
    bar_->setTextVisible(true);
    bar_->setAccessibleName(QStringLiteral("Task progress"));
    cancel_ = new QPushButton(QStringLiteral("Cancel"), this);
    cancel_->setObjectName(QStringLiteral("cancelTask"));
    cancel_->setAccessibleName(QStringLiteral("Cancel task"));
    cancel_->setEnabled(false);
    row->addWidget(bar_, 1);
    row->addWidget(cancel_);
    connect(cancel_, &QPushButton::clicked, this, &TaskProgress::cancelled);
}

void TaskProgress::setRunning(bool running) {
    running_ = running;
    cancel_->setEnabled(running);
}

void TaskProgress::setPercent(int percent) {
    if (percent < 0) {
        percent_ = -1;
        bar_->setRange(0, 0);
        return;
    }
    percent_ = std::clamp(percent, 0, 100);
    bar_->setRange(0, 100);
    bar_->setValue(percent_);
}

int TaskProgress::percent() const {
    return percent_;
}

bool TaskProgress::isRunning() const {
    return running_;
}

Banner::Banner(QWidget* parent) : QWidget(parent) {
    setProperty("mfRole", QStringLiteral("banner"));
    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(medforge::ui::tokens::space12,
                            medforge::ui::tokens::space8,
                            medforge::ui::tokens::space8,
                            medforge::ui::tokens::space8);
    row->setSpacing(medforge::ui::tokens::space8);
    severityLabel_ = new QLabel(this);
    severityLabel_->setMinimumWidth(72);
    messageLabel_ = new QLabel(this);
    messageLabel_->setWordWrap(true);
    messageLabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    dismiss_ = new QToolButton(this);
    dismiss_->setProperty("mfRole", QStringLiteral("inline"));
    dismiss_->setObjectName(QStringLiteral("dismissBanner"));
    dismiss_->setIcon(medforge::ui::icons::clear());
    dismiss_->setAutoRaise(true);
    dismiss_->setAccessibleName(QStringLiteral("Dismiss notification"));
    row->addWidget(severityLabel_);
    row->addWidget(messageLabel_, 1);
    row->addWidget(dismiss_);
    connect(dismiss_, &QToolButton::clicked, this, &Banner::dismissed);
    setMessage(Severity::Notice, QString());
}

void Banner::setMessage(Severity severity, const QString& message) {
    severity_ = severity;
    const QString key = severityKey(severity);
    setProperty("mfSeverity", key);
    severityLabel_->setProperty("mfSeverity", key);
    severityLabel_->setText(severityText());
    messageLabel_->setText(message);
    repolish(this);
    repolish(severityLabel_);
}

void Banner::setDismissible(bool dismissible) {
    dismiss_->setVisible(dismissible);
}

QString Banner::severityText() const {
    switch (severity_) {
    case Severity::Warning:
        return QStringLiteral("Warning");
    case Severity::Error:
        return QStringLiteral("Error");
    case Severity::Success:
        return QStringLiteral("Success");
    case Severity::Notice:
        break;
    }
    return QStringLiteral("Notice");
}

QString Banner::message() const {
    return messageLabel_->text();
}

Banner::Severity Banner::severity() const {
    return severity_;
}

EmptyState::EmptyState(const QString& title,
                       const QString& body,
                       const QString& actionLabel,
                       QWidget* parent)
    : QWidget(parent) {
    auto* column = new QVBoxLayout(this);
    column->setContentsMargins(medforge::ui::tokens::space16,
                               medforge::ui::tokens::space16,
                               medforge::ui::tokens::space16,
                               medforge::ui::tokens::space16);
    column->setSpacing(medforge::ui::tokens::space8);
    auto* titleLabel = new QLabel(title, this);
    titleLabel->setProperty("mfRole", QStringLiteral("section"));
    titleLabel->setWordWrap(true);
    auto* bodyLabel = new QLabel(body, this);
    bodyLabel->setProperty("mfRole", QStringLiteral("secondary"));
    bodyLabel->setWordWrap(true);
    column->addWidget(titleLabel);
    column->addWidget(bodyLabel);
    if (!actionLabel.isEmpty()) {
        auto* action = new QPushButton(actionLabel, this);
        action->setProperty("mfRole", QStringLiteral("primary"));
        action->setAccessibleName(actionLabel);
        column->addWidget(action, 0, Qt::AlignLeft);
        connect(action, &QPushButton::clicked, this, &EmptyState::actionTriggered);
    }
}

ValidationMessage::ValidationMessage(QWidget* parent) : QLabel(parent) {
    setProperty("mfRole", QStringLiteral("error"));
    setWordWrap(true);
}

void ValidationMessage::setError(const QString& fieldName, const QString& message) {
    setText(QStringLiteral("Error: %1 — %2").arg(fieldName, message));
}
