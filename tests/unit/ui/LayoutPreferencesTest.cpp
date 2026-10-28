#include "medforge/ui/LayoutPreferences.h"

#include <QDir>
#include <QFile>
#include <QtTest>

class LayoutPreferencesTest : public QObject {
    Q_OBJECT

private slots:
    void roundTripIsSeparateFromProjectFiles();
    void missingFileDoesNotInventALayout();
    void resetRemovesTheSavedLayout();
};

void LayoutPreferencesTest::roundTripIsSeparateFromProjectFiles() {
    const QString path = QDir::temp().filePath(QStringLiteral("medforge-m1-layout.ini"));
    QFile::remove(path);
    LayoutPreferences preferences(path);
    QVERIFY(!preferences.path().contains(QStringLiteral("project"), Qt::CaseInsensitive));
    const QByteArray state("dock-state");
    const QByteArray geometry("window-geometry");
    preferences.save(state, geometry);
    QByteArray loadedState;
    QByteArray loadedGeometry;
    QVERIFY(preferences.load(&loadedState, &loadedGeometry));
    QCOMPARE(loadedState, state);
    QCOMPARE(loadedGeometry, geometry);
    QVERIFY(preferences.hasSavedLayout());
}

void LayoutPreferencesTest::missingFileDoesNotInventALayout() {
    const QString path = QDir::temp().filePath(QStringLiteral("medforge-m1-missing.ini"));
    QFile::remove(path);
    const LayoutPreferences preferences(path);
    QByteArray state;
    QByteArray geometry;
    QVERIFY(!preferences.load(&state, &geometry));
    QVERIFY(state.isEmpty());
    QVERIFY(!preferences.hasSavedLayout());
}

void LayoutPreferencesTest::resetRemovesTheSavedLayout() {
    const QString path = QDir::temp().filePath(QStringLiteral("medforge-m1-reset.ini"));
    QFile::remove(path);
    LayoutPreferences preferences(path);
    preferences.save(QByteArray("state"), QByteArray("geometry"));
    preferences.reset();
    QVERIFY(!preferences.hasSavedLayout());
    QByteArray state(4, 'x');
    QVERIFY(!preferences.load(&state, nullptr));
    QVERIFY(state.isEmpty());
}

QTEST_APPLESS_MAIN(LayoutPreferencesTest)
#include "LayoutPreferencesTest.moc"
