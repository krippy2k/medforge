#pragma once

#include <QDialog>

class QPushButton;

class ConfirmDialog : public QDialog {
public:
    ConfirmDialog(const QString& title,
                  const QString& message,
                  const QString& confirmLabel,
                  QWidget* parent = nullptr);

    static bool
    ask(QWidget* parent, const QString& title, const QString& message, const QString& confirmLabel);
};
