#include "medforge/core/EntityId.h"

#include <QtTest>

using medforge::core::EntityId;

class EntityIdTest : public QObject {
    Q_OBJECT

private slots:
    void zeroIsInvalid();
    void roundTripsHex();
    void rejectsMalformedText();
    void generateIsValidAndStableAcrossString();
};

void EntityIdTest::zeroIsInvalid() {
    QCOMPARE(EntityId{}.valid(), false);
    QCOMPARE(EntityId::fromValue(0).valid(), false);
    QCOMPARE(EntityId::fromString("0").valid(), false);
}

void EntityIdTest::roundTripsHex() {
    const EntityId id = EntityId::fromValue(0x1a2b);
    QCOMPARE(QString::fromStdString(id.toString()), QStringLiteral("0000000000001a2b"));
    QCOMPARE(EntityId::fromString(id.toString()), id);
}

void EntityIdTest::rejectsMalformedText() {
    QVERIFY(!EntityId::fromString("").valid());
    QVERIFY(!EntityId::fromString("zz").valid());
    QVERIFY(!EntityId::fromString("1234567890abcdef0").valid());
}

void EntityIdTest::generateIsValidAndStableAcrossString() {
    const EntityId id = EntityId::generate();
    QVERIFY(id.valid());
    QCOMPARE(EntityId::fromString(id.toString()), id);
}

QTEST_APPLESS_MAIN(EntityIdTest)
#include "EntityIdTest.moc"
