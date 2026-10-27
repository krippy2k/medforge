#pragma once

#include <QList>
#include <QStringList>
#include <QWidget>

class QKeyEvent;
class QPushButton;

class SegmentedControl : public QWidget {
    Q_OBJECT

public:
    explicit SegmentedControl(const QStringList& labels, QWidget* parent = nullptr);

    int currentIndex() const;
    int count() const;
    bool setCurrentIndex(int index);

signals:
    void currentIndexChanged(int index);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    int current_ = 0;
    QList<QPushButton*> buttons_;
};
