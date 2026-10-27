#include "medforge/ui/ScreenshotCapture.h"

#include "medforge/ui/ConfirmDialog.h"
#include "medforge/ui/DesignTokens.h"
#include "medforge/ui/FoundationWindow.h"
#include "medforge/ui/GalleryWindow.h"
#include "medforge/ui/WorkspaceReference.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QTextStream>
#include <QWidget>
#include <algorithm>

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
                QTextStream& manifest) {
    widget.setMinimumSize(0, 0);
    widget.resize(logicalSize);
    widget.show();
    QApplication::processEvents();
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
    return true;
}

} // namespace medforge::ui
