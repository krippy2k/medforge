#include "medforge/ui/SegmentedControl.h"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QPushButton>

SegmentedControl::SegmentedControl(const QStringList& labels, QWidget* parent) : QWidget(parent) {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    setFocusPolicy(Qt::StrongFocus);

    int index = 0;
    for (const QString& label : labels) {
        auto* button = new QPushButton(label, this);
        button->setProperty("mfRole", QStringLiteral("segment"));
        button->setAccessibleName(label);
        button->setCheckable(true);
        button->setFocusPolicy(Qt::NoFocus);
        button->setMinimumHeight(32);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        const int captured = index;
        connect(
            button, &QPushButton::clicked, this, [this, captured]() { setCurrentIndex(captured); });
        layout->addWidget(button);
        buttons_.push_back(button);
        ++index;
    }
    if (!buttons_.isEmpty()) {
        buttons_.first()->setChecked(true);
    }
}

int SegmentedControl::currentIndex() const {
    return current_;
}

int SegmentedControl::count() const {
    return static_cast<int>(buttons_.size());
}

bool SegmentedControl::setCurrentIndex(int index) {
    if (!isEnabled() || index < 0 || index >= count()) {
        return false;
    }
    if (index == current_) {
        return true;
    }
    current_ = index;
    for (int i = 0; i < count(); ++i) {
        buttons_.at(i)->setChecked(i == current_);
    }
    emit currentIndexChanged(current_);
    return true;
}

void SegmentedControl::keyPressEvent(QKeyEvent* event) {
    if (!isEnabled()) {
        event->ignore();
        return;
    }
    if (event->key() == Qt::Key_Right || event->key() == Qt::Key_Down) {
        if (current_ + 1 < count()) {
            setCurrentIndex(current_ + 1);
        }
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Up) {
        if (current_ > 0) {
            setCurrentIndex(current_ - 1);
        }
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}
