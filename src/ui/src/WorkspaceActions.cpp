#include "medforge/ui/WorkspaceActions.h"

WorkspaceActions::WorkspaceActions(QObject* parent) : QObject(parent) {}

WorkspaceActions::Phase WorkspaceActions::phase() const {
    return phase_;
}

bool WorkspaceActions::canShowSample() const {
    return phase_ != Phase::Ready;
}

bool WorkspaceActions::canResetCamera() const {
    return phase_ == Phase::Ready;
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
        return QStringLiteral("Renderer error");
    }
    return {};
}

QString WorkspaceActions::errorText() const {
    return errorText_;
}

void WorkspaceActions::showSample() {
    errorText_.clear();
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
    phase_ = Phase::Error;
    emit phaseChanged();
}
