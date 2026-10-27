#include "medforge/ui/ConfirmDialog.h"

#include "medforge/ui/DesignTokens.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

ConfirmDialog::ConfirmDialog(const QString& title,
                             const QString& message,
                             const QString& confirmLabel,
                             QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(title);
    setModal(true);
    setMinimumWidth(420);
    auto* column = new QVBoxLayout(this);
    column->setContentsMargins(medforge::ui::tokens::space16,
                               medforge::ui::tokens::space16,
                               medforge::ui::tokens::space16,
                               medforge::ui::tokens::space16);
    column->setSpacing(medforge::ui::tokens::space12);
    auto* heading = new QLabel(title, this);
    heading->setProperty("mfRole", QStringLiteral("section"));
    heading->setWordWrap(true);
    auto* body = new QLabel(message, this);
    body->setWordWrap(true);
    column->addWidget(heading);
    column->addWidget(body);

    auto* buttons = new QHBoxLayout();
    buttons->addStretch(1);
    auto* cancel = new QPushButton(QStringLiteral("Cancel"), this);
    cancel->setObjectName(QStringLiteral("cancelButton"));
    cancel->setAccessibleName(QStringLiteral("Cancel"));
    cancel->setDefault(true);
    cancel->setAutoDefault(true);
    auto* confirm = new QPushButton(confirmLabel, this);
    confirm->setObjectName(QStringLiteral("confirmButton"));
    confirm->setAccessibleName(confirmLabel);
    confirm->setAutoDefault(false);
    confirm->setProperty("mfRole", QStringLiteral("destructive"));
    buttons->addWidget(cancel);
    buttons->addWidget(confirm);
    column->addLayout(buttons);

    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(confirm, &QPushButton::clicked, this, &QDialog::accept);
}

bool ConfirmDialog::ask(QWidget* parent,
                        const QString& title,
                        const QString& message,
                        const QString& confirmLabel) {
    ConfirmDialog dialog(title, message, confirmLabel, parent);
    return dialog.exec() == QDialog::Accepted;
}
