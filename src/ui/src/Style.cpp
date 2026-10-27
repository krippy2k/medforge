#include "medforge/ui/Style.h"

#include "medforge/ui/DesignTokens.h"

#include <QApplication>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QPalette>
#include <QStyleFactory>

namespace medforge::ui {
namespace {

void replaceColor(QString& sheet, const char* token, const QColor& color) {
    sheet.replace(QLatin1String(token), color.name(QColor::HexRgb));
}

} // namespace

QString buildStyleSheet() {
    QString sheet = QStringLiteral(R"qss(
QWidget {
    background: @background@;
    color: @textPrimary@;
    font-family: "Segoe UI", "DejaVu Sans", sans-serif;
    font-size: @fontPointSize@pt;
}
QLabel { background: transparent; border: none; }
QLabel[mfRole="section"] { font-size: @sectionPointSize@pt; color: @textPrimary@; }
QLabel[mfRole="secondary"], QLabel[mfRole="annotation"] { color: @textSecondary@; }
QLabel[mfRole="error"] { color: @error@; }
QWidget[mfRole="sectionFrame"], QFrame[mfRole="dock"], QFrame[mfRole="viewport"], QFrame[mfRole="toolbar"] {
    background: @panel@;
    border: 1px solid @border@;
}
QFrame[mfRole="viewport"] { background: @raised@; }
QWidget[mfRole="dockHeader"], QStatusBar, QMenuBar {
    background: @raised@;
    color: @textPrimary@;
}
QMenuBar::item:selected, QMenu::item:selected { background: @selected@; }
QMenu { background: @panel@; border: 1px solid @border@; }
QStatusBar { color: @textSecondary@; }
QPushButton, QToolButton {
    background: @raised@;
    color: @textPrimary@;
    border: 1px solid @border@;
    padding: 4px 12px;
    min-height: @controlHeight@px;
}
QToolButton { padding: 4px; min-width: @controlHeight@px; }
QToolButton[mfRole="inline"] {
    background: transparent;
    border: 1px solid transparent;
    min-width: 28px;
    min-height: 28px;
    padding: 2px;
}
QPushButton[mfRole="primary"] {
    background: @accent@;
    color: @ink@;
    border: 1px solid @accent@;
}
QPushButton[mfRole="destructive"] {
    background: @error@;
    color: @ink@;
    border: 1px solid @error@;
}
QPushButton[mfRole="segment"] {
    border-radius: 0px;
    background: @panel@;
    min-height: @controlHeight@px;
}
QPushButton[mfRole="segment"]:checked {
    background: @selected@;
    border: 1px solid @accent@;
    color: @textPrimary@;
}
QPushButton[mfRole="segment"]:disabled,
QPushButton[mfRole="segment"]:checked:disabled {
    background: @panel@;
    color: @textDisabled@;
    border: 1px solid @border@;
}
QPushButton:hover, QToolButton:hover { border-color: @accent@; }
QPushButton:focus, QToolButton:focus, QLineEdit:focus {
    border: 1px solid @accent@;
}
QPushButton:disabled, QToolButton:disabled, QLineEdit:disabled {
    color: @textDisabled@;
    background: @panel@;
    border: 1px solid @border@;
}
QLineEdit {
    background: @background@;
    color: @textPrimary@;
    border: 1px solid @border@;
    padding: 4px 8px;
    min-height: 24px;
    selection-background-color: @selected@;
}
QLineEdit[mfState="invalid"] { border: 1px solid @error@; }
QWidget[mfRole="browserRow"] { background: transparent; border: none; }
QWidget[mfRole="browserRow"][mfSelected="true"] { background: @selected@; }
QWidget[mfRole="banner"] { border: 1px solid @border@; }
QWidget[mfRole="banner"][mfSeverity="notice"] { background: #1c2c36; border-color: @accent@; }
QWidget[mfRole="banner"][mfSeverity="warning"] { background: #3a3424; border-color: @warning@; }
QWidget[mfRole="banner"][mfSeverity="error"] { background: #3a2428; border-color: @error@; }
QWidget[mfRole="banner"][mfSeverity="success"] { background: #1d3330; border-color: @success@; }
QLabel[mfSeverity="notice"] { color: @accent@; background: transparent; }
QLabel[mfSeverity="warning"] { color: @warning@; background: transparent; }
QLabel[mfSeverity="error"] { color: @error@; background: transparent; }
QLabel[mfSeverity="success"] { color: @success@; background: transparent; }
QProgressBar {
    background: @background@;
    border: 1px solid @border@;
    color: @textPrimary@;
    text-align: center;
    min-height: 16px;
}
QProgressBar::chunk { background: @accent@; }
QScrollArea, QScrollArea > QWidget > QWidget { background: @background@; border: none; }
QScrollBar:vertical { background: @background@; width: 12px; margin: 0; }
QScrollBar::handle:vertical { background: @border@; min-height: 24px; }
QScrollBar:horizontal { background: @background@; height: 12px; }
QScrollBar::handle:horizontal { background: @border@; min-width: 24px; }
QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; }
QDialog { background: @panel@; }
)qss");

    replaceColor(sheet, "@background@", tokens::background);
    replaceColor(sheet, "@raised@", tokens::raised);
    replaceColor(sheet, "@panel@", tokens::panel);
    replaceColor(sheet, "@border@", tokens::border);
    replaceColor(sheet, "@textPrimary@", tokens::textPrimary);
    replaceColor(sheet, "@textSecondary@", tokens::textSecondary);
    replaceColor(sheet, "@textDisabled@", tokens::textDisabled);
    replaceColor(sheet, "@accent@", tokens::accent);
    replaceColor(sheet, "@selected@", tokens::selected);
    replaceColor(sheet, "@warning@", tokens::warning);
    replaceColor(sheet, "@error@", tokens::error);
    replaceColor(sheet, "@success@", tokens::success);
    replaceColor(sheet, "@ink@", tokens::ink);
    sheet.replace(QStringLiteral("@controlHeight@"), QString::number(tokens::controlHeight));
    sheet.replace(QStringLiteral("@fontPointSize@"), QString::number(tokens::fontPointSize));
    sheet.replace(QStringLiteral("@sectionPointSize@"), QString::number(tokens::sectionPointSize));
    return sheet;
}

void loadBundledFonts() {
    const QStringList files = {
        QStringLiteral("C:/Windows/Fonts/segoeui.ttf"),
        QStringLiteral("C:/Windows/Fonts/segoeuib.ttf"),
        QStringLiteral("C:/Windows/Fonts/consola.ttf"),
        QStringLiteral("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"),
        QStringLiteral("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf"),
        QStringLiteral("/usr/share/fonts/dejavu/DejaVuSans.ttf"),
        QStringLiteral("/usr/share/fonts/dejavu/DejaVuSansMono.ttf")};
    for (const QString& file : files) {
        if (QFile::exists(file)) {
            QFontDatabase::addApplicationFont(file);
        }
    }
}

void applyMedForgeStyle(QApplication& application) {
    if (QStyle* fusion = QStyleFactory::create(QStringLiteral("Fusion"))) {
        application.setStyle(fusion);
    }
    QStringList families = QFontDatabase::families();
    QFont font;
    if (families.contains(tokens::fontFamily)) {
        font = QFont(tokens::fontFamily);
    } else if (families.contains(QStringLiteral("DejaVu Sans"))) {
        font = QFont(QStringLiteral("DejaVu Sans"));
    } else {
        font = application.font();
    }
    font.setPointSize(tokens::fontPointSize);
    application.setFont(font);

    QPalette palette = application.palette();
    palette.setColor(QPalette::Window, tokens::background);
    palette.setColor(QPalette::WindowText, tokens::textPrimary);
    palette.setColor(QPalette::Base, tokens::background);
    palette.setColor(QPalette::AlternateBase, tokens::panel);
    palette.setColor(QPalette::Text, tokens::textPrimary);
    palette.setColor(QPalette::Button, tokens::raised);
    palette.setColor(QPalette::ButtonText, tokens::textPrimary);
    palette.setColor(QPalette::Highlight, tokens::selected);
    palette.setColor(QPalette::HighlightedText, tokens::textPrimary);
    palette.setColor(QPalette::PlaceholderText, tokens::textSecondary);
    palette.setColor(QPalette::Disabled, QPalette::Text, tokens::textDisabled);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, tokens::textDisabled);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, tokens::textDisabled);
    application.setPalette(palette);
    application.setStyleSheet(buildStyleSheet());
}

} // namespace medforge::ui
