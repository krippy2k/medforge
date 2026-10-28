#include "medforge/ui/ScreenshotCapture.h"

#include "medforge/imaging/DicomWrite.h"
#include "medforge/imaging/SyntheticPhantom.h"
#include "medforge/ui/ConfirmDialog.h"
#include "medforge/ui/DesignTokens.h"
#include "medforge/ui/FoundationWindow.h"
#include "medforge/ui/GalleryWindow.h"
#include "medforge/ui/FoundationWidgets.h"
#include "medforge/ui/NumericEditor.h"
#include "medforge/ui/SegmentedControl.h"
#include "medforge/ui/VolumeViewport.h"
#include "medforge/ui/WorkspaceActions.h"
#include "medforge/ui/WorkspaceReference.h"
#include "medforge/ui/WorkspaceWindow.h"

#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>

#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QTextStream>
#include <QWidget>
#include <algorithm>
#include <filesystem>
#include <functional>

namespace medforge::ui {
namespace {

int scalePercent() {
    bool ok = false;
    const double scale = qEnvironmentVariable("QT_SCALE_FACTOR", QStringLiteral("1")).toDouble(&ok);
    if (!ok || !(scale > 0.0)) {
        return 100;
    }
    return qRound(scale * 100.0);
}

bool savePixmap(const QPixmap& pixmap, const QString& path, QString* error) {
    if (pixmap.isNull() || pixmap.width() < 16 || pixmap.height() < 16) {
        if (error != nullptr) {
            *error = QStringLiteral("Grab failed for %1").arg(path);
        }
        return false;
    }
    if (!pixmap.save(path, "PNG")) {
        if (error != nullptr) {
            *error = QStringLiteral("Could not write %1").arg(path);
        }
        return false;
    }
    return true;
}

bool saveWidget(QWidget& widget,
                const QSize& logicalSize,
                const QString& path,
                QString* error,
                QTextStream& manifest,
                const std::function<void()>& beforeGrab = {}) {
    widget.setMinimumSize(0, 0);
    widget.resize(logicalSize);
    widget.show();
    QApplication::processEvents();
    if (beforeGrab) {
        beforeGrab();
        QApplication::processEvents();
    }
    widget.repaint();
    QApplication::processEvents();
    const QPixmap pixmap = widget.grab();
    manifest << QFileInfo(path).fileName() << " logical " << logicalSize.width() << "x"
             << logicalSize.height() << " image " << pixmap.width() << "x" << pixmap.height()
             << " dpr " << widget.devicePixelRatioF() << "\n";
    return savePixmap(pixmap, path, error);
}

bool saveDialog(const QSize& canvasSize,
                const QString& path,
                QString* error,
                QTextStream& manifest) {
    QWidget backdrop;
    backdrop.resize(canvasSize);
    backdrop.show();
    QApplication::processEvents();
    const QPixmap canvas = backdrop.grab();

    ConfirmDialog dialog(QStringLiteral("Delete landmark"),
                         QStringLiteral("Delete the selected landmark from this research document? "
                                        "This sample does not delete data."),
                         QStringLiteral("Delete"));
    dialog.setMinimumWidth(440);
    dialog.adjustSize();
    dialog.show();
    QApplication::processEvents();
    const QPixmap dialogPixels = dialog.grab();

    QImage base = canvas.toImage();
    base.setDevicePixelRatio(1.0);
    QImage dialogImage = dialogPixels.toImage();
    dialogImage.setDevicePixelRatio(1.0);
    const int maxWidth = base.width() * 8 / 10;
    const int maxHeight = base.height() * 7 / 10;
    if (dialogImage.width() > maxWidth || dialogImage.height() > maxHeight) {
        dialogImage =
            dialogImage.scaled(maxWidth, maxHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    const int x = std::max(0, (base.width() - dialogImage.width()) / 2);
    const int y = std::max(0, (base.height() - dialogImage.height()) / 3);
    QPainter painter(&base);
    painter.fillRect(base.rect(), tokens::background);
    painter.fillRect(
        QRect(x, y, dialogImage.width(), dialogImage.height()).adjusted(-16, -16, 16, 16),
        QColor(0, 0, 0, 140));
    painter.drawImage(x, y, dialogImage);
    painter.end();
    dialog.close();
    backdrop.close();
    manifest << QFileInfo(path).fileName() << " logical " << canvasSize.width() << "x"
             << canvasSize.height() << " image " << base.width() << "x" << base.height() << "\n";
    if (!base.save(path, "PNG")) {
        if (error != nullptr) {
            *error = QStringLiteral("Could not write %1").arg(path);
        }
        return false;
    }
    return true;
}

bool waitUntil(int timeoutMs, const std::function<bool()>& ready) {
    QElapsedTimer timer;
    timer.start();
    while (!ready() && timer.elapsed() < timeoutMs) {
        QApplication::processEvents(QEventLoop::AllEvents, 20);
    }
    return ready();
}

bool waitForPhase(WorkspaceWindow& window, WorkspaceActions::Phase phase, int timeoutMs) {
    QElapsedTimer timer;
    timer.start();
    while (window.actions()->phase() != phase && timer.elapsed() < timeoutMs) {
        QApplication::processEvents(QEventLoop::AllEvents, 20);
    }
    return window.actions()->phase() == phase;
}

QString pathString(const std::filesystem::path& path) {
#ifdef Q_OS_WIN
    return QString::fromStdWString(path.wstring());
#else
    return QString::fromStdString(path.string());
#endif
}

bool saveMprStates(const QString& directory,
                   int percent,
                   QString* error,
                   QTextStream& manifest) {
    const QString suffix = QStringLiteral("-1280x800-scale%1.png").arg(percent);
    const QString layout = QDir::temp().filePath(QStringLiteral("medforge-m2-capture-layout.ini"));
    {
        WorkspaceWindow empty(layout);
        if (!saveWidget(empty,
                        QSize(1280, 800),
                        QDir(directory).filePath(QStringLiteral("mpr-empty") + suffix),
                        error,
                        manifest)) {
            return false;
        }
        empty.close();
    }
    {
        const std::filesystem::path folder =
            std::filesystem::temp_directory_path() / "medforge-m2-capture-invalid";
        std::filesystem::remove_all(folder);
        std::filesystem::create_directories(folder);
        medforge::imaging::DicomCtInstance slice;
        slice.status = medforge::imaging::DicomReadStatus::Ok;
        slice.seriesInstanceUid = "1.2.826.0.1.3680043.10.543.200";
        slice.geometry.imageOrientationPatient = {1, 0, 0, 0, 1, 0};
        slice.geometry.rowSpacingMm = 1;
        slice.geometry.columnSpacingMm = 1;
        slice.rescale = medforge::imaging::Rescale{1, 0};
        slice.rows = 2;
        slice.columns = 2;
        slice.storedValues = {0, 0, 0, 0};
        std::string writeError;
        if (!medforge::imaging::writePart10Ct(folder / "a.dcm", slice, 1, &writeError) ||
            !medforge::imaging::writePart10Ct(folder / "b.dcm", slice, 2, &writeError)) {
            if (error != nullptr) {
                *error = QString::fromStdString(writeError);
            }
            return false;
        }
        WorkspaceWindow invalid(layout);
        invalid.importFolder(pathString(folder));
        if (!waitForPhase(invalid, WorkspaceActions::Phase::Error, 5000)) {
            if (error != nullptr) {
                *error = QStringLiteral("Invalid series did not report an error.");
            }
            return false;
        }
        if (!saveWidget(invalid,
                        QSize(1280, 800),
                        QDir(directory).filePath(QStringLiteral("mpr-invalid") + suffix),
                        error,
                        manifest)) {
            return false;
        }
        invalid.close();
    }

    const std::filesystem::path phantom =
        std::filesystem::temp_directory_path() / "medforge-m2-capture-phantom";
    std::string phantomError;
    if (!medforge::imaging::phantom::writeSyntheticAxialSeries(phantom, &phantomError)) {
        if (error != nullptr) {
            *error = QString::fromStdString(phantomError);
        }
        return false;
    }
    {
        WorkspaceWindow importing(layout);
        importing.importFolder(pathString(phantom), 250);
        if (!saveWidget(importing,
                        QSize(1280, 800),
                        QDir(directory).filePath(QStringLiteral("mpr-importing") + suffix),
                        error,
                        manifest)) {
            return false;
        }
        importing.close();
    }
    const QList<QSize> sizes = {QSize(1280, 800), QSize(1920, 1080), QSize(2560, 1440)};
    for (const QSize& size : sizes) {
        WorkspaceWindow populated(layout);
        populated.importFolder(pathString(phantom));
        if (!waitForPhase(populated, WorkspaceActions::Phase::Study, 5000)) {
            if (error != nullptr) {
                *error = QStringLiteral("Synthetic CT import did not finish.");
            }
            return false;
        }
        const QString name =
            QStringLiteral("mpr-populated-%1x%2-scale%3.png")
                .arg(size.width())
                .arg(size.height())
                .arg(percent);
        if (!saveWidget(populated, size, QDir(directory).filePath(name), error, manifest)) {
            return false;
        }
        populated.close();
    }
    return true;
}

bool saveVolumeStates(const QString& directory,
                      int percent,
                      QString* error,
                      QTextStream& manifest) {
    const std::filesystem::path phantom =
        std::filesystem::temp_directory_path() / "medforge-m3-capture-phantom";
    std::string phantomError;
    if (!medforge::imaging::phantom::writeSyntheticAxialSeries(phantom, &phantomError)) {
        if (error != nullptr) {
            *error = QString::fromStdString(phantomError);
        }
        return false;
    }
    const QString layout = QDir::temp().filePath(QStringLiteral("medforge-m3-capture-layout.ini"));
    const QString path = pathString(phantom);
    {
        WorkspaceWindow building(layout);
        building.importFolder(path);
        if (!waitForPhase(building, WorkspaceActions::Phase::Study, 5000)) {
            if (error != nullptr) {
                *error = QStringLiteral("Volume capture import did not finish.");
            }
            return false;
        }
        building.buildSurface(1500);
        if (!saveWidget(building,
                        QSize(1280, 800),
                        QDir(directory).filePath(
                            QStringLiteral("volume-building-1280x800-scale%1.png").arg(percent)),
                        error,
                        manifest)) {
            return false;
        }
        building.close();
    }
    const QList<QSize> sizes = {QSize(1280, 800), QSize(1920, 1080), QSize(2560, 1440)};
    for (const QSize& size : sizes) {
        WorkspaceWindow populated(layout);
        populated.importFolder(path);
        if (!waitForPhase(populated, WorkspaceActions::Phase::Study, 5000)) {
            if (error != nullptr) {
                *error = QStringLiteral("Volume capture import did not finish.");
            }
            return false;
        }
        const QString name = QStringLiteral("volume-populated-%1x%2-scale%3.png")
                                 .arg(size.width())
                                 .arg(size.height())
                                 .arg(percent);
        if (!saveWidget(populated, size, QDir(directory).filePath(name), error, manifest)) {
            return false;
        }
        populated.close();
    }
    {
        WorkspaceWindow emptySurface(layout);
        emptySurface.importFolder(path);
        if (!waitForPhase(emptySurface, WorkspaceActions::Phase::Study, 5000)) {
            if (error != nullptr) {
                *error = QStringLiteral("Volume capture import did not finish.");
            }
            return false;
        }
        auto* threshold = emptySurface.findChild<NumericEditor*>(QStringLiteral("isoThreshold"));
        if (threshold == nullptr) {
            if (error != nullptr) {
                *error = QStringLiteral("Threshold control is missing.");
            }
            return false;
        }
        threshold->setValue(3000);
        emptySurface.buildSurface();
        if (!waitUntil(5000, [&emptySurface] {
                return emptySurface.errorBanner()->message().contains(QStringLiteral("No surface")) ||
                       emptySurface.errorBanner()->message().contains(QStringLiteral("VTK"));
            })) {
            if (error != nullptr) {
                *error = QStringLiteral("Empty threshold did not report a result.");
            }
            return false;
        }
        if (!saveWidget(emptySurface,
                        QSize(1280, 800),
                        QDir(directory).filePath(
                            QStringLiteral("volume-no-surface-1280x800-scale%1.png").arg(percent)),
                        error,
                        manifest)) {
            return false;
        }
        emptySurface.close();
    }
    for (const QSize& size : sizes) {
        WorkspaceWindow meshed(layout);
        meshed.importFolder(path);
        if (!waitForPhase(meshed, WorkspaceActions::Phase::Study, 5000)) {
            if (error != nullptr) {
                *error = QStringLiteral("Volume capture import did not finish.");
            }
            return false;
        }
        auto* threshold = meshed.findChild<NumericEditor*>(QStringLiteral("isoThreshold"));
        if (threshold == nullptr) {
            if (error != nullptr) {
                *error = QStringLiteral("Threshold control is missing.");
            }
            return false;
        }
        threshold->setValue(0);
        meshed.buildSurface();
        const bool ready = waitUntil(5000, [&meshed] {
            return meshed.volumeViewport()->meshTriangleCount() > 0 ||
                   meshed.errorBanner()->message().contains(QStringLiteral("VTK"));
        });
        if (!ready) {
            if (error != nullptr) {
                *error = QStringLiteral("Surface extraction did not finish.");
            }
            return false;
        }
        const QString name = QStringLiteral("volume-mesh-%1x%2-scale%3.png")
                                 .arg(size.width())
                                 .arg(size.height())
                                 .arg(percent);
        if (!saveWidget(meshed, size, QDir(directory).filePath(name), error, manifest)) {
            return false;
        }
        meshed.close();
    }
    return true;
}

void revealSegmentControls(WorkspaceWindow& window) {
    auto* scroll = window.findChild<QScrollArea*>(QStringLiteral("propertiesScroll"));
    auto* anchor = window.findChild<QWidget*>(QStringLiteral("segmentTool"));
    if (scroll == nullptr || anchor == nullptr || scroll->widget() == nullptr) {
        return;
    }
    const QPoint top = anchor->mapTo(scroll->widget(), QPoint(0, 0));
    scroll->verticalScrollBar()->setValue(top.y());
}

bool saveSegmentStates(const QString& directory,
                       int percent,
                       QString* error,
                       QTextStream& manifest) {
    const std::filesystem::path phantom =
        std::filesystem::temp_directory_path() / "medforge-m4-capture-phantom";
    std::string phantomError;
    if (!medforge::imaging::phantom::writeSyntheticAxialSeries(phantom, &phantomError)) {
        if (error != nullptr) {
            *error = QString::fromStdString(phantomError);
        }
        return false;
    }
    const QString layout = QDir::temp().filePath(QStringLiteral("medforge-m4-capture-layout.ini"));
    const QString path = pathString(phantom);
    const QList<QSize> sizes = {QSize(1280, 800), QSize(1920, 1080), QSize(2560, 1440)};
    for (const QSize& size : sizes) {
        WorkspaceWindow segmented(layout);
        segmented.importFolder(path);
        if (!waitForPhase(segmented, WorkspaceActions::Phase::Study, 5000)) {
            if (error != nullptr) {
                *error = QStringLiteral("Segmentation capture import did not finish.");
            }
            return false;
        }
        auto* labels = segmented.findChild<SegmentedControl*>(QStringLiteral("segmentLabel"));
        if (labels == nullptr || !labels->setCurrentIndex(0)) {
            if (error != nullptr) {
                *error = QStringLiteral("Label control is missing.");
            }
            return false;
        }
        segmented.applySegmentThreshold();
        segmented.focusLabeledVoxel();
        const QString name = QStringLiteral("segment-overlay-%1x%2-scale%3.png")
                                 .arg(size.width())
                                 .arg(size.height())
                                 .arg(percent);
        if (!saveWidget(segmented, size, QDir(directory).filePath(name), error, manifest,
                        [&segmented] { revealSegmentControls(segmented); })) {
            return false;
        }
        segmented.close();
    }
    {
        WorkspaceWindow otherLabel(layout);
        otherLabel.importFolder(path);
        if (!waitForPhase(otherLabel, WorkspaceActions::Phase::Study, 5000)) {
            if (error != nullptr) {
                *error = QStringLiteral("Segmentation capture import did not finish.");
            }
            return false;
        }
        auto* labels = otherLabel.findChild<SegmentedControl*>(QStringLiteral("segmentLabel"));
        if (labels == nullptr || !labels->setCurrentIndex(0)) {
            if (error != nullptr) {
                *error = QStringLiteral("Label control is missing.");
            }
            return false;
        }
        otherLabel.applySegmentThreshold();
        otherLabel.focusLabeledVoxel();
        if (!labels->setCurrentIndex(1)) {
            if (error != nullptr) {
                *error = QStringLiteral("Label control is missing.");
            }
            return false;
        }
        if (!saveWidget(otherLabel,
                        QSize(1280, 800),
                        QDir(directory).filePath(
                            QStringLiteral("segment-label2-1280x800-scale%1.png").arg(percent)),
                        error,
                        manifest,
                        [&otherLabel] { revealSegmentControls(otherLabel); })) {
            return false;
        }
        otherLabel.close();
    }
    for (const QSize& size : sizes) {
        WorkspaceWindow meshed(layout);
        meshed.importFolder(path);
        if (!waitForPhase(meshed, WorkspaceActions::Phase::Study, 5000)) {
            if (error != nullptr) {
                *error = QStringLiteral("Segmentation capture import did not finish.");
            }
            return false;
        }
        auto* labels = meshed.findChild<SegmentedControl*>(QStringLiteral("segmentLabel"));
        if (labels == nullptr || !labels->setCurrentIndex(0)) {
            if (error != nullptr) {
                *error = QStringLiteral("Label control is missing.");
            }
            return false;
        }
        meshed.applySegmentThreshold();
        meshed.focusLabeledVoxel();
        meshed.buildLabelMesh();
        const bool ready = waitUntil(5000, [&meshed] {
            return meshed.volumeViewport()->meshTriangleCount() > 0 ||
                   meshed.errorBanner()->message().contains(QStringLiteral("VTK")) ||
                   meshed.errorBanner()->message().contains(QStringLiteral("empty"));
        });
        if (!ready) {
            if (error != nullptr) {
                *error = QStringLiteral("Label mesh did not finish.");
            }
            return false;
        }
        const QString name = QStringLiteral("segment-mesh-%1x%2-scale%3.png")
                                 .arg(size.width())
                                 .arg(size.height())
                                 .arg(percent);
        if (!saveWidget(meshed, size, QDir(directory).filePath(name), error, manifest,
                        [&meshed] { revealSegmentControls(meshed); })) {
            return false;
        }
        meshed.close();
    }
    WorkspaceWindow missed(layout);
    missed.importFolder(path);
    if (!waitForPhase(missed, WorkspaceActions::Phase::Study, 5000)) {
        if (error != nullptr) {
            *error = QStringLiteral("Segmentation capture import did not finish.");
        }
        return false;
    }
    missed.growFromCrosshair();
    if (!waitUntil(5000, [&missed] {
            return !missed.errorBanner()->isHidden() &&
                   missed.errorBanner()->message().contains(QStringLiteral("Seed"));
        })) {
        if (error != nullptr) {
            *error = QStringLiteral("Crosshair grow did not report the seed.");
        }
        return false;
    }
    return saveWidget(missed,
                      QSize(1280, 800),
                      QDir(directory).filePath(QStringLiteral("segment-seed-1280x800-scale%1.png").arg(percent)),
                      error,
                      manifest,
                      [&missed] { revealSegmentControls(missed); });
}

} // namespace

bool captureUiEvidence(const QString& directory, QString* error) {
    QDir().mkpath(directory);
    const int percent = scalePercent();
    QFile manifestFile(
        QDir(directory).filePath(QStringLiteral("manifest-scale%1.txt").arg(percent)));
    if (!manifestFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        if (error != nullptr) {
            *error = QStringLiteral("Could not write the capture manifest.");
        }
        return false;
    }
    QTextStream manifest(&manifestFile);
    const QList<QSize> sizes = {QSize(1280, 800), QSize(1920, 1080), QSize(2560, 1440)};
    for (const QSize& size : sizes) {
        const QString suffix =
            QStringLiteral("-%1x%2-scale%3.png").arg(size.width()).arg(size.height()).arg(percent);
        GalleryWindow gallery;
        if (!saveWidget(gallery,
                        size,
                        QDir(directory).filePath(QStringLiteral("gallery") + suffix),
                        error,
                        manifest)) {
            return false;
        }
        gallery.close();
        WorkspaceReference reference;
        if (!saveWidget(reference,
                        size,
                        QDir(directory).filePath(QStringLiteral("reference") + suffix),
                        error,
                        manifest)) {
            return false;
        }
        reference.close();
    }
    const QSize dialogCanvas(1280, 800);
    if (!saveDialog(
            dialogCanvas,
            QDir(directory).filePath(QStringLiteral("confirm-1280x800-scale%1.png").arg(percent)),
            error,
            manifest)) {
        return false;
    }
    FoundationWindow foundation;
    if (!saveWidget(foundation,
                    QSize(1280, 800),
                    QDir(directory).filePath(
                        QStringLiteral("foundation-1280x800-scale%1.png").arg(percent)),
                    error,
                    manifest)) {
        return false;
    }
    foundation.close();
    return saveMprStates(directory, percent, error, manifest) &&
           saveVolumeStates(directory, percent, error, manifest) &&
           saveSegmentStates(directory, percent, error, manifest);
}

} // namespace medforge::ui
