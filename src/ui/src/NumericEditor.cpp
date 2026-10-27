#include "medforge/ui/NumericEditor.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QStyle>
#include <QVBoxLayout>
#include <cmath>

NumericEditor::NumericEditor(QWidget* parent) : QWidget(parent) {
    auto* column = new QVBoxLayout(this);
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(4);

    auto* row = new QHBoxLayout();
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(8);
    label_ = new QLabel(this);
    label_->setObjectName(QStringLiteral("numericLabel"));
    edit_ = new QLineEdit(this);
    edit_->setObjectName(QStringLiteral("numericValue"));
    edit_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QFont numericFont;
    numericFont.setFamilies({QStringLiteral("Consolas"), QStringLiteral("DejaVu Sans Mono")});
    numericFont.setStyleHint(QFont::Monospace);
    numericFont.setPointSize(10);
    edit_->setFont(numericFont);
    unit_ = new QLabel(this);
    unit_->setObjectName(QStringLiteral("numericUnit"));
    row->addWidget(label_);
    row->addWidget(edit_, 1);
    row->addWidget(unit_);

    errorLabel_ = new QLabel(this);
    errorLabel_->setObjectName(QStringLiteral("numericError"));
    errorLabel_->setProperty("mfRole", QStringLiteral("error"));
    errorLabel_->setWordWrap(true);
    errorLabel_->setVisible(false);
    column->addLayout(row);
    column->addWidget(errorLabel_);

    connect(edit_, &QLineEdit::editingFinished, this, &NumericEditor::commitText);
}

void NumericEditor::setLabel(const QString& label) {
    label_->setText(label);
    edit_->setAccessibleName(label);
}

void NumericEditor::setUnit(const QString& unit) {
    unit_->setText(unit);
}

void NumericEditor::setRange(double minimum, double maximum) {
    minimum_ = minimum;
    maximum_ = maximum;
    hasRange_ = true;
}

void NumericEditor::setDecimals(int decimals) {
    decimals_ = decimals;
}

double NumericEditor::value() const {
    return value_;
}

void NumericEditor::setValue(double value) {
    if (!std::isfinite(value) || !withinRange(value)) {
        publishValidation(false, rangeMessage());
        return;
    }
    value_ = value;
    showValue();
    publishValidation(true, {});
    emit valueChanged(value_);
}

bool NumericEditor::isValid() const {
    return error_.isEmpty();
}

QString NumericEditor::validationError() const {
    return error_;
}

void NumericEditor::commitText() {
    if (!isEnabled()) {
        showValue();
        return;
    }
    const QString text = edit_->text().trimmed();
    if (text.isEmpty()) {
        publishValidation(false, QStringLiteral("Enter a number"));
        return;
    }
    bool ok = false;
    const double parsed = QLocale::c().toDouble(text, &ok);
    if (!ok || !std::isfinite(parsed)) {
        publishValidation(false, QStringLiteral("Enter a number"));
        return;
    }
    if (!withinRange(parsed)) {
        publishValidation(false, rangeMessage());
        return;
    }
    value_ = parsed;
    showValue();
    publishValidation(true, {});
    emit valueChanged(value_);
}

void NumericEditor::showValue() {
    edit_->setText(QString::number(value_, 'f', decimals_));
}

void NumericEditor::publishValidation(bool valid, const QString& error) {
    error_ = error;
    errorLabel_->setText(error);
    errorLabel_->setVisible(!valid);
    edit_->setProperty("mfState", valid ? QVariant() : QVariant(QStringLiteral("invalid")));
    edit_->style()->unpolish(edit_);
    edit_->style()->polish(edit_);
    emit validationChanged(valid);
}

QString NumericEditor::rangeMessage() const {
    QString unitSuffix;
    if (!unit_->text().isEmpty()) {
        unitSuffix = QStringLiteral(" ") + unit_->text();
    }
    return QStringLiteral("Value must be between %1 and %2%3")
        .arg(QString::number(minimum_, 'g', 12), QString::number(maximum_, 'g', 12), unitSuffix);
}

bool NumericEditor::withinRange(double value) const {
    if (!hasRange_) {
        return true;
    }
    return value >= minimum_ && value <= maximum_;
}
