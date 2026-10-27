#include "medforge/ui/FoundationWidgets.h"

#include "medforge/ui/ConfirmDialog.h"

#include <QPushButton>
#include <QSignalSpy>
#include <QToolButton>
#include <QtTest>

class FoundationWidgetsTest : public QObject {
    Q_OBJECT

private slots:
    void bannerNamesSeverityInText();
    void taskCancelEmitsOnlyWhileRunning();
    void propertyGroupHidesContentWhenCollapsed();
    void confirmDialogCancelRejects();
};

void FoundationWidgetsTest::bannerNamesSeverityInText() {
    Banner banner;
    banner.setMessage(Banner::Severity::Error, QStringLiteral("Import failed"));
    QCOMPARE(banner.severityText(), QStringLiteral("Error"));
    QVERIFY(banner.message().contains(QStringLiteral("Import failed")));
    QSignalSpy spy(&banner, &Banner::dismissed);
    auto* dismiss = banner.findChild<QToolButton*>(QStringLiteral("dismissBanner"));
    QVERIFY(dismiss != nullptr);
    banner.show();
    QApplication::processEvents();
    QTest::mouseClick(dismiss, Qt::LeftButton);
    QCOMPARE(spy.count(), 1);
}

void FoundationWidgetsTest::taskCancelEmitsOnlyWhileRunning() {
    TaskProgress progress;
    progress.resize(320, 40);
    progress.show();
    QApplication::processEvents();
    QSignalSpy spy(&progress, &TaskProgress::cancelled);
    auto* cancel = progress.findChild<QPushButton*>(QStringLiteral("cancelTask"));
    QVERIFY(cancel != nullptr);
    QVERIFY(!cancel->isEnabled());
    QTest::mouseClick(cancel, Qt::LeftButton);
    QCOMPARE(spy.count(), 0);

    progress.setRunning(true);
    progress.setPercent(40);
    QCOMPARE(progress.percent(), 40);
    QVERIFY(cancel->isEnabled());
    QTest::mouseClick(cancel, Qt::LeftButton);
    QCOMPARE(spy.count(), 1);
}

void FoundationWidgetsTest::propertyGroupHidesContentWhenCollapsed() {
    PropertyGroup group(QStringLiteral("Spacing"));
    auto* row = new QWidget(&group);
    row->setMinimumSize(40, 20);
    group.addRow(row);
    group.resize(240, 160);
    group.show();
    QApplication::processEvents();
    QVERIFY(row->isVisible());
    group.setExpanded(false);
    QVERIFY(!row->isVisible());
    group.setExpanded(true);
    QVERIFY(row->isVisible());
}

void FoundationWidgetsTest::confirmDialogCancelRejects() {
    ConfirmDialog dialog(
        QStringLiteral("Delete landmark"), QStringLiteral("Delete it?"), QStringLiteral("Delete"));
    dialog.show();
    QApplication::processEvents();
    auto* cancel = dialog.findChild<QPushButton*>(QStringLiteral("cancelButton"));
    auto* confirm = dialog.findChild<QPushButton*>(QStringLiteral("confirmButton"));
    QVERIFY(cancel != nullptr);
    QVERIFY(confirm != nullptr);
    QVERIFY(cancel->isDefault());
    QTest::mouseClick(cancel, Qt::LeftButton);
    QCOMPARE(dialog.result(), static_cast<int>(QDialog::Rejected));
}

QTEST_MAIN(FoundationWidgetsTest)
#include "FoundationWidgetsTest.moc"
