#include "medforge/core/Cancellation.h"

#include <QtTest>

using medforge::core::CancellationSource;
using medforge::core::CancellationToken;

class CancellationTest : public QObject {
    Q_OBJECT

private slots:
    void copiesObserveCancel();
    void sourcesAreIndependent();
};

void CancellationTest::copiesObserveCancel() {
    CancellationSource source;
    const CancellationToken first = source.token();
    const CancellationToken second = source.token();
    QVERIFY(!first.isCancelled());
    QVERIFY(!second.isCancelled());
    source.cancel();
    QVERIFY(first.isCancelled());
    QVERIFY(second.isCancelled());
}

void CancellationTest::sourcesAreIndependent() {
    CancellationSource left;
    CancellationSource right;
    left.cancel();
    QVERIFY(left.token().isCancelled());
    QVERIFY(!right.token().isCancelled());
}

QTEST_APPLESS_MAIN(CancellationTest)
#include "CancellationTest.moc"
