#include "medforge/ui/WorkspaceWindow.h"

#include "medforge/rendering/CameraPose.h"
#include "medforge/ui/FoundationWidgets.h"
#include "medforge/ui/LayoutPreferences.h"
#include "medforge/ui/SampleViewport.h"

#include <QApplication>
#include <QDir>
#include <QDockWidget>
#include <QFile>
#include <QThread>
#include <QtTest>
#include <atomic>

class WorkspaceWindowTest : public QObject {
    Q_OBJECT

private slots:
    void titleShowsTheActiveMilestone();
    void restoresHiddenDockAndResetClearsIt();
    void showSampleEnablesResetCamera();
    void showSampleKeepsTheSameNativeWindow();
    void viewportAcceptsFocus();
    void workerThreadCannotApplyAPose();
};

void WorkspaceWindowTest::titleShowsTheActiveMilestone() {
    const QString path = QDir::temp().filePath(QStringLiteral("medforge-m2-title.ini"));
    QFile::remove(path);
    WorkspaceWindow window(path);
    QCOMPARE(window.windowTitle(), QStringLiteral("MedForge M6"));
}

void WorkspaceWindowTest::restoresHiddenDockAndResetClearsIt() {
    const QString path = QDir::temp().filePath(QStringLiteral("medforge-m1-window.ini"));
    QFile::remove(path);
    {
        WorkspaceWindow window(path);
        QVERIFY(!window.studiesDock()->isHidden());
        QVERIFY(!window.propertiesDock()->isHidden());
        QVERIFY(!window.tasksDock()->isHidden());
        window.propertiesDock()->hide();
        window.saveLayout();
    }
    WorkspaceWindow restored(path);
    QVERIFY(restored.propertiesDock()->isHidden());
    QVERIFY(!restored.studiesDock()->isHidden());
    restored.resetLayout();
    QVERIFY(!restored.propertiesDock()->isHidden());
    QVERIFY(!LayoutPreferences(path).hasSavedLayout());
}

void WorkspaceWindowTest::showSampleEnablesResetCamera() {
    const QString path = QDir::temp().filePath(QStringLiteral("medforge-m1-actions.ini"));
    QFile::remove(path);
    WorkspaceWindow window(path);
    QVERIFY(!window.resetCameraAction()->isEnabled());
    QVERIFY(window.showSampleAction()->isEnabled());
    window.showSample();
    QVERIFY(window.sampleViewport()->sampleVisible());
    QVERIFY(window.resetCameraAction()->isEnabled());
    QVERIFY(!window.showSampleAction()->isEnabled());
    window.resetCameraAction()->trigger();
    QVERIFY(medforge::rendering::cameraPosesNear(
        window.sampleViewport()->pose(), medforge::rendering::homeCamera(), 1e-9));
    QCOMPARE(window.errorBanner()->isVisible(), false);
}

void WorkspaceWindowTest::showSampleKeepsTheSameNativeWindow() {
    if (qEnvironmentVariable("QT_QPA_PLATFORM") == QLatin1String("offscreen")) {
        QSKIP("Native window identity is not meaningful offscreen");
    }
    const QString path = QDir::temp().filePath(QStringLiteral("medforge-m1-native.ini"));
    QFile::remove(path);
    WorkspaceWindow window(path);
    window.show();
    QApplication::processEvents();
    const WId before = window.winId();
    window.showSample();
    QApplication::processEvents();
    QCOMPARE(window.winId(), before);
}

void WorkspaceWindowTest::viewportAcceptsFocus() {
    const QString path = QDir::temp().filePath(QStringLiteral("medforge-m1-focus.ini"));
    QFile::remove(path);
    WorkspaceWindow window(path);
    window.show();
    window.activateWindow();
    window.sampleViewport()->setFocus(Qt::OtherFocusReason);
    QApplication::processEvents();
    QCOMPARE(QApplication::focusWidget(), static_cast<QWidget*>(window.sampleViewport()));
}

void WorkspaceWindowTest::workerThreadCannotApplyAPose() {
    SampleViewport viewport;
    std::atomic<bool> accepted{true};
    QString error;
    auto* thread = QThread::create([&viewport, &accepted, &error] {
        accepted = viewport.applyPose(medforge::rendering::homeCamera(), &error);
    });
    thread->start();
    QVERIFY(thread->wait(2000));
    delete thread;
    QVERIFY(!accepted);
    QCOMPARE(error, QStringLiteral("Rendering must run on the GUI thread"));
}

QTEST_MAIN(WorkspaceWindowTest)
#include "WorkspaceWindowTest.moc"
