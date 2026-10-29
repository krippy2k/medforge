#include "medforge/cad/Module.h"
#include "medforge/core/Version.h"
#include "medforge/geometry/Module.h"
#include "medforge/imaging/Module.h"
#include "medforge/rendering/Module.h"

#include <QtTest>

class ModulesTest : public QObject {
    Q_OBJECT

private slots:
    void moduleNamesFollowDependencyDirection();
    void researchNoticeIsVisibleText();
};

void ModulesTest::moduleNamesFollowDependencyDirection() {
    QCOMPARE(medforge::core::moduleName(), "core");
    QCOMPARE(medforge::imaging::moduleName(), "imaging");
    QCOMPARE(medforge::geometry::moduleName(), "geometry");
    QCOMPARE(medforge::cad::moduleName(), "cad");
    QCOMPARE(medforge::rendering::moduleName(), "rendering");
}

void ModulesTest::researchNoticeIsVisibleText() {
    const QString notice = QString::fromUtf8(medforge::core::kResearchNotice);
    QVERIFY(notice.contains(QStringLiteral("not for clinical use")));
    QCOMPARE(QString::fromUtf8(medforge::core::kVersionString), QStringLiteral("0.1.0"));
    QCOMPARE(QString::fromUtf8(medforge::core::kMilestone), QStringLiteral("M8"));
}

QTEST_APPLESS_MAIN(ModulesTest)
#include "ModulesTest.moc"
