#include "medforge/ui/WorkspaceActions.h"

WorkspaceActions::WorkspaceActions(QObject* parent) : QObject(parent) {}

WorkspaceActions::Phase WorkspaceActions::phase() const {
    return phase_;
}

bool WorkspaceActions::canShowSample() const {
    return phase_ == Phase::Empty || phase_ == Phase::Error;
}

bool WorkspaceActions::canResetCamera() const {
    return phase_ == Phase::Ready || phase_ == Phase::Study;
}

bool WorkspaceActions::canResetLayout() const {
    return true;
}

QString WorkspaceActions::statusText() const {
    switch (phase_) {
    case Phase::Empty:
        return QStringLiteral("No dataset loaded");
    case Phase::Ready:
        return QStringLiteral("Sample cube and sphere");
    case Phase::Error:
        return importError_ ? QStringLiteral("Import failed") : QStringLiteral("Renderer error");
    case Phase::Importing:
        return QStringLiteral("Importing CT series");
    case Phase::Study:
        return studyStatus_;
    }
    return {};
}

QString WorkspaceActions::errorText() const {
    return errorText_;
}

void WorkspaceActions::showSample() {
    errorText_.clear();
    importError_ = false;
    phase_ = Phase::Ready;
    emit phaseChanged();
}

bool WorkspaceActions::resetCamera() {
    if (!canResetCamera()) {
        return false;
    }
    emit cameraResetRequested();
    return true;
}

void WorkspaceActions::reportError(const QString& message) {
    errorText_ = message;
    importError_ = false;
    phase_ = Phase::Error;
    emit phaseChanged();
}

void WorkspaceActions::beginImport() {
    errorText_.clear();
    importError_ = false;
    phase_ = Phase::Importing;
    emit phaseChanged();
}

void WorkspaceActions::showStudy(const QString& status) {
    errorText_.clear();
    importError_ = false;
    studyStatus_ = status;
    phase_ = Phase::Study;
    emit phaseChanged();
}

void WorkspaceActions::failImport(const QString& message) {
    errorText_ = message;
    importError_ = true;
    phase_ = Phase::Error;
    emit phaseChanged();
}

void WorkspaceActions::noteCancelled() {
    errorText_.clear();
    importError_ = false;
    phase_ = Phase::Empty;
    emit phaseChanged();
}
