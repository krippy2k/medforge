#include "medforge/ui/WorkspaceActions.h"

#include <QSignalSpy>
#include <QtTest>

class WorkspaceActionsTest : public QObject {
    Q_OBJECT

private slots:
    void emptyWorkspaceDisablesResetCamera();
    void showSampleEnablesResetAndClearsErrors();
    void resetCameraIsIgnoredUntilSampleIsVisible();
    void errorDisablesResetAndKeepsTheMessage();
};

void WorkspaceActionsTest::emptyWorkspaceDisablesResetCamera() {
    const WorkspaceActions actions;
    QCOMPARE(actions.phase(), WorkspaceActions::Phase::Empty);
    QVERIFY(actions.canShowSample());
    QVERIFY(!actions.canResetCamera());
    QVERIFY(actions.canResetLayout());
    QCOMPARE(actions.statusText(), QStringLiteral("No dataset loaded"));
}

void WorkspaceActionsTest::showSampleEnablesResetAndClearsErrors() {
    WorkspaceActions actions;
    actions.reportError(QStringLiteral("Renderer failed"));
    QSignalSpy spy(&actions, &WorkspaceActions::phaseChanged);
    actions.showSample();
    QCOMPARE(actions.phase(), WorkspaceActions::Phase::Ready);
    QVERIFY(actions.canResetCamera());
    QCOMPARE(actions.errorText(), QString());
    QCOMPARE(actions.statusText(), QStringLiteral("Sample cube and sphere"));
    QVERIFY(spy.count() > 0);
}

void WorkspaceActionsTest::resetCameraIsIgnoredUntilSampleIsVisible() {
    WorkspaceActions actions;
    QSignalSpy spy(&actions, &WorkspaceActions::cameraResetRequested);
    QVERIFY(!actions.resetCamera());
    QCOMPARE(spy.count(), 0);
    actions.showSample();
    QVERIFY(actions.resetCamera());
    QCOMPARE(spy.count(), 1);
}

void WorkspaceActionsTest::errorDisablesResetAndKeepsTheMessage() {
    WorkspaceActions actions;
    actions.showSample();
    actions.reportError(QStringLiteral("OpenGL context was not created"));
    QCOMPARE(actions.phase(), WorkspaceActions::Phase::Error);
    QVERIFY(!actions.canResetCamera());
    QVERIFY(actions.canShowSample());
    QCOMPARE(actions.errorText(), QStringLiteral("OpenGL context was not created"));
    QCOMPARE(actions.statusText(), QStringLiteral("Renderer error"));
}

QTEST_APPLESS_MAIN(WorkspaceActionsTest)
#include "WorkspaceActionsTest.moc"
