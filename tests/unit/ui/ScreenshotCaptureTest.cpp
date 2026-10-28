#include "medforge/ui/ScreenshotCapture.h"

#include "medforge/ui/Style.h"

#include <QDir>
#include <QFileInfo>
#include <QtTest>

class ScreenshotCaptureTest : public QObject {
    Q_OBJECT

private slots:
    void writesGalleryAndReferencePngs();
};

void ScreenshotCaptureTest::writesGalleryAndReferencePngs() {
    medforge::ui::loadBundledFonts();
    medforge::ui::applyMedForgeStyle(*qApp);
    const QString directory = QDir::temp().filePath(QStringLiteral("medforge-m0-capture"));
    QDir(directory).removeRecursively();
    QString error;
    QVERIFY2(medforge::ui::captureUiEvidence(directory, &error), qPrintable(error));
    const QStringList required = {
        QStringLiteral("gallery-1280x800-scale100.png"),
        QStringLiteral("gallery-1920x1080-scale100.png"),
        QStringLiteral("gallery-2560x1440-scale100.png"),
        QStringLiteral("reference-1280x800-scale100.png"),
        QStringLiteral("reference-1920x1080-scale100.png"),
        QStringLiteral("reference-2560x1440-scale100.png"),
        QStringLiteral("confirm-1280x800-scale100.png"),
        QStringLiteral("foundation-1280x800-scale100.png"),
        QStringLiteral("mpr-empty-1280x800-scale100.png"),
        QStringLiteral("mpr-invalid-1280x800-scale100.png"),
        QStringLiteral("mpr-importing-1280x800-scale100.png"),
        QStringLiteral("mpr-populated-1280x800-scale100.png"),
        QStringLiteral("mpr-populated-1920x1080-scale100.png"),
        QStringLiteral("mpr-populated-2560x1440-scale100.png"),
        QStringLiteral("volume-building-1280x800-scale100.png"),
        QStringLiteral("volume-populated-1280x800-scale100.png"),
        QStringLiteral("segment-overlay-1280x800-scale100.png"),
        QStringLiteral("segment-overlay-1920x1080-scale100.png"),
        QStringLiteral("segment-overlay-2560x1440-scale100.png"),
        QStringLiteral("segment-seed-1280x800-scale100.png"),
        QStringLiteral("segment-label2-1280x800-scale100.png"),
        QStringLiteral("segment-mesh-1280x800-scale100.png"),
        QStringLiteral("segment-mesh-1920x1080-scale100.png"),
        QStringLiteral("segment-mesh-2560x1440-scale100.png"),
        QStringLiteral("volume-populated-1920x1080-scale100.png"),
        QStringLiteral("volume-populated-2560x1440-scale100.png"),
        QStringLiteral("volume-no-surface-1280x800-scale100.png"),
        QStringLiteral("volume-mesh-1280x800-scale100.png"),
        QStringLiteral("volume-mesh-1920x1080-scale100.png"),
        QStringLiteral("volume-mesh-2560x1440-scale100.png"),
    };
    for (const QString& name : required) {
        const QFileInfo info(QDir(directory).filePath(name));
        QVERIFY2(info.exists(), qPrintable(name));
        QVERIFY2(info.size() > 1000, qPrintable(name));
    }
}

QTEST_MAIN(ScreenshotCaptureTest)
#include "ScreenshotCaptureTest.moc"
