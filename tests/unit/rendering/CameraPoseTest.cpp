#include "medforge/rendering/CameraPose.h"

#include <QtTest>
#include <cmath>

using medforge::rendering::cameraPosesNear;
using medforge::rendering::homeCamera;
using medforge::rendering::orbitYaw;
using medforge::rendering::pan;
using medforge::rendering::zoom;

class CameraPoseTest : public QObject {
    Q_OBJECT

private slots:
    void homeLooksAtOriginFromAboveTheFront();
    void orbitYawMovesTheCameraAndKeepsTheFocalPoint();
    void panMovesCameraAndFocalPointTogether();
    void zoomMovesCloserWithoutChangingTheFocalPoint();
    void resetReturnsHomeAfterNavigation();
};

void CameraPoseTest::homeLooksAtOriginFromAboveTheFront() {
    const auto home = homeCamera();
    QCOMPARE(home.focalPoint[0], 0.0);
    QCOMPARE(home.focalPoint[1], 0.0);
    QCOMPARE(home.focalPoint[2], 0.0);
    QVERIFY(home.position[2] > 0.0);
    QVERIFY(home.position[1] < 0.0);
    QCOMPARE(home.viewUp[2], 1.0);
}

void CameraPoseTest::orbitYawMovesTheCameraAndKeepsTheFocalPoint() {
    const auto home = homeCamera();
    const auto turned = orbitYaw(home, 90.0);
    QCOMPARE(turned.focalPoint, home.focalPoint);
    QVERIFY(std::abs(turned.position[0] - home.position[0]) > 0.5);
}

void CameraPoseTest::panMovesCameraAndFocalPointTogether() {
    const auto home = homeCamera();
    const auto shifted = pan(home, 2.0, 0.0);
    const double cameraDx = shifted.position[0] - home.position[0];
    const double focalDx = shifted.focalPoint[0] - home.focalPoint[0];
    QVERIFY(std::abs(cameraDx) > 0.5);
    QVERIFY(std::abs(cameraDx - focalDx) < 1e-9);
}

void CameraPoseTest::zoomMovesCloserWithoutChangingTheFocalPoint() {
    const auto home = homeCamera();
    const auto closer = zoom(home, 2.0);
    QCOMPARE(closer.focalPoint, home.focalPoint);
    const auto distance = [](const auto& pose) {
        double sum = 0;
        for (int axis = 0; axis < 3; ++axis) {
            const double delta = pose.position[static_cast<std::size_t>(axis)] -
                                 pose.focalPoint[static_cast<std::size_t>(axis)];
            sum += delta * delta;
        }
        return std::sqrt(sum);
    };
    QVERIFY(distance(closer) < distance(home) * 0.6);
}

void CameraPoseTest::resetReturnsHomeAfterNavigation() {
    const auto home = homeCamera();
    const auto moved = zoom(orbitYaw(pan(home, 1.0, -1.0), 30.0), 1.5);
    QVERIFY(!cameraPosesNear(moved, home, 1e-6));
    QVERIFY(cameraPosesNear(homeCamera(), home, 1e-9));
}

QTEST_APPLESS_MAIN(CameraPoseTest)
#include "CameraPoseTest.moc"
