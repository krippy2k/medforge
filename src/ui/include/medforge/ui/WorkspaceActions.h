#pragma once

#include <QObject>
#include <QString>

class WorkspaceActions : public QObject {
    Q_OBJECT

public:
    enum class Phase { Empty, Ready, Error };

    explicit WorkspaceActions(QObject* parent = nullptr);

    Phase phase() const;
    bool canShowSample() const;
    bool canResetCamera() const;
    bool canResetLayout() const;
    QString statusText() const;
    QString errorText() const;

    void showSample();
    bool resetCamera();
    void reportError(const QString& message);

signals:
    void phaseChanged();
    void cameraResetRequested();

private:
    Phase phase_ = Phase::Empty;
    QString errorText_;
};
