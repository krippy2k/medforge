#pragma once

#include <QColor>
#include <QString>

// Token values match spec/DESIGN_SYSTEM.md. Layout code and the stylesheet both read these.
namespace medforge::ui::tokens {

inline const QColor background{0x15, 0x19, 0x1F};
inline const QColor raised{0x20, 0x26, 0x2E};
inline const QColor panel{0x25, 0x2D, 0x36};
inline const QColor border{0x3A, 0x46, 0x52};
inline const QColor textPrimary{0xE9, 0xEF, 0xF5};
inline const QColor textSecondary{0xAF, 0xBB, 0xC8};
inline const QColor textDisabled{0x8B, 0x98, 0xA6};
inline const QColor accent{0x4B, 0xA3, 0xC7};
inline const QColor selected{0x23, 0x4B, 0x60};
inline const QColor warning{0xE7, 0xB7, 0x5D};
inline const QColor error{0xE6, 0x7E, 0x80};
inline const QColor success{0x65, 0xBF, 0xA1};
inline const QColor ink{0x15, 0x19, 0x1F};

inline constexpr int space4 = 4;
inline constexpr int space8 = 8;
inline constexpr int space12 = 12;
inline constexpr int space16 = 16;
inline constexpr int space24 = 24;
inline constexpr int controlHeight = 32;
inline constexpr int fontPointSize = 10;
inline constexpr int sectionPointSize = 12;

inline const QString fontFamily = QStringLiteral("Segoe UI");
inline const QString numericFontFamily = QStringLiteral("Consolas");

} // namespace medforge::ui::tokens
