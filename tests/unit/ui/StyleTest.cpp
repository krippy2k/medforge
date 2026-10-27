#include "medforge/ui/Style.h"

#include "medforge/ui/DesignTokens.h"

#include <QtTest>

class StyleTest : public QObject {
    Q_OBJECT

private slots:
    void tokensDriveTheStylesheet();
};

void StyleTest::tokensDriveTheStylesheet() {
    QCOMPARE(medforge::ui::tokens::accent.name(QColor::HexRgb), QStringLiteral("#4ba3c7"));
    QCOMPARE(medforge::ui::tokens::background.name(QColor::HexRgb), QStringLiteral("#15191f"));
    QCOMPARE(medforge::ui::tokens::controlHeight, 32);
    const QString sheet = medforge::ui::buildStyleSheet();
    QVERIFY(sheet.contains(QStringLiteral("#4ba3c7")));
    QVERIFY(sheet.contains(QStringLiteral("#15191f")));
    QVERIFY(sheet.contains(QStringLiteral("min-height: 32px")));
    QVERIFY(!sheet.contains(QStringLiteral("@accent@")));
}

QTEST_APPLESS_MAIN(StyleTest)
#include "StyleTest.moc"
