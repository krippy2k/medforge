#include "medforge/ui/LayoutPreferences.h"

#include <QFile>
#include <QSettings>

namespace {
constexpr int kSchema = 1;
}

LayoutPreferences::LayoutPreferences(const QString& filePath) : path_(filePath) {}

QString LayoutPreferences::path() const {
    return path_;
}

void LayoutPreferences::save(const QByteArray& state, const QByteArray& geometry) {
    QSettings settings(path_, QSettings::IniFormat);
    settings.setValue(QStringLiteral("schema"), kSchema);
    settings.setValue(QStringLiteral("state"), state);
    settings.setValue(QStringLiteral("geometry"), geometry);
    settings.sync();
}

bool LayoutPreferences::load(QByteArray* state, QByteArray* geometry) const {
    if (state != nullptr) {
        state->clear();
    }
    if (geometry != nullptr) {
        geometry->clear();
    }
    if (!QFile::exists(path_)) {
        return false;
    }
    const QSettings settings(path_, QSettings::IniFormat);
    if (settings.value(QStringLiteral("schema")).toInt() != kSchema) {
        return false;
    }
    const QByteArray loadedState = settings.value(QStringLiteral("state")).toByteArray();
    if (loadedState.isEmpty()) {
        return false;
    }
    if (state != nullptr) {
        *state = loadedState;
    }
    if (geometry != nullptr) {
        *geometry = settings.value(QStringLiteral("geometry")).toByteArray();
    }
    return true;
}

void LayoutPreferences::reset() {
    QFile::remove(path_);
}

bool LayoutPreferences::hasSavedLayout() const {
    return QFile::exists(path_);
}
