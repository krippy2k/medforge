#include "medforge/ui/SegmentedControl.h"

#include <QSignalSpy>
#include <QtTest>

class SegmentedControlTest : public QObject {
    Q_OBJECT

private slots:
    void startsOnFirstSegment();
    void enabledSelectionEmitsSignal();
    void rejectsOutOfRangeIndex();
    void disabledFreezesSelection();
    void keyboardChangesSelectionOnlyWhenEnabled();
};

void SegmentedControlTest::startsOnFirstSegment() {
    const SegmentedControl control(
        {QStringLiteral("Axial"), QStringLiteral("Sagittal"), QStringLiteral("Coronal")});
    QCOMPARE(control.count(), 3);
    QCOMPARE(control.currentIndex(), 0);
}

void SegmentedControlTest::enabledSelectionEmitsSignal() {
    SegmentedControl control(
        {QStringLiteral("Axial"), QStringLiteral("Sagittal"), QStringLiteral("Coronal")});
    QSignalSpy spy(&control, &SegmentedControl::currentIndexChanged);
    QVERIFY(control.setCurrentIndex(2));
    QCOMPARE(control.currentIndex(), 2);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toInt(), 2);
}

void SegmentedControlTest::rejectsOutOfRangeIndex() {
    SegmentedControl control({QStringLiteral("Axial"), QStringLiteral("Sagittal")});
    QVERIFY(!control.setCurrentIndex(-1));
    QVERIFY(!control.setCurrentIndex(2));
    QCOMPARE(control.currentIndex(), 0);
}

void SegmentedControlTest::disabledFreezesSelection() {
    SegmentedControl control(
        {QStringLiteral("Axial"), QStringLiteral("Sagittal"), QStringLiteral("Coronal")});
    QSignalSpy spy(&control, &SegmentedControl::currentIndexChanged);
    control.setEnabled(false);
    QVERIFY(!control.setCurrentIndex(2));
    QCOMPARE(control.currentIndex(), 0);
    QCOMPARE(spy.count(), 0);
}

void SegmentedControlTest::keyboardChangesSelectionOnlyWhenEnabled() {
    SegmentedControl control(
        {QStringLiteral("Axial"), QStringLiteral("Sagittal"), QStringLiteral("Coronal")});
    control.resize(320, 36);
    control.show();
    QApplication::processEvents();

    QTest::keyClick(&control, Qt::Key_Right);
    QCOMPARE(control.currentIndex(), 1);
    QTest::keyClick(&control, Qt::Key_Left);
    QCOMPARE(control.currentIndex(), 0);

    control.setCurrentIndex(1);
    control.setEnabled(false);
    QTest::keyClick(&control, Qt::Key_Right);
    QCOMPARE(control.currentIndex(), 1);
}

QTEST_MAIN(SegmentedControlTest)
#include "SegmentedControlTest.moc"
