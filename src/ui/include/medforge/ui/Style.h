#pragma once

#include <QString>

class QApplication;

namespace medforge::ui {

QString buildStyleSheet();
void loadBundledFonts();
void applyMedForgeStyle(QApplication& application);

} // namespace medforge::ui
