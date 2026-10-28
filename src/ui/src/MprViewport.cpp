#include "medforge/ui/MprViewport.h"

#include "medforge/ui/DesignTokens.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>

MprViewport::MprViewport(const QString& title, QWidget* parent) : QWidget(parent), title_(title) {
    setProperty("mfRole", QStringLiteral("viewport"));
    setFocusPolicy(Qt::StrongFocus);
    setAccessibleName(title);
    setMinimumSize(160, 120);
    setMouseTracking(false);
}

void MprViewport::setImage(const QImage& image) {
    image_ = image;
    update();
}

void MprViewport::setMillimetersPerPixel(double xMm, double yMm) {
    xMm_ = xMm > 0 ? xMm : 1;
    yMm_ = yMm > 0 ? yMm : 1;
    update();
}

void MprViewport::clearImage() {
    image_ = QImage();
    xMm_ = 1;
    yMm_ = 1;
    update();
}

void MprViewport::setCrosshair(const QPoint& pixel) {
    crosshair_ = pixel;
    update();
}

void MprViewport::setOrientation(const QString& left,
                                 const QString& right,
                                 const QString& top,
                                 const QString& bottom) {
    left_ = left;
    right_ = right;
    top_ = top;
    bottom_ = bottom;
    update();
}

bool MprViewport::hasImage() const {
    return !image_.isNull() && image_.width() > 0 && image_.height() > 0;
}

QSize MprViewport::imagePixelSize() const {
    return image_.size();
}

QRect MprViewport::imageBounds() const {
    return imageRect();
}

QPoint MprViewport::crosshair() const {
    return crosshair_;
}

int MprViewport::grayAt(int x, int y) const {
    if (!hasImage() || x < 0 || y < 0 || x >= image_.width() || y >= image_.height()) {
        return -1;
    }
    return image_.pixelColor(x, y).value();
}

QString MprViewport::planeTitle() const {
    return title_;
}

QString MprViewport::leftLabel() const {
    return left_;
}

QString MprViewport::rightLabel() const {
    return right_;
}

QString MprViewport::topLabel() const {
    return top_;
}

QString MprViewport::bottomLabel() const {
    return bottom_;
}

QRect MprViewport::imageRect() const {
    const QRect bounds = rect().adjusted(22, 28, -22, -22);
    if (!hasImage() || bounds.width() < 2 || bounds.height() < 2) {
        return {};
    }
    const double physicalWidth = image_.width() * xMm_;
    const double physicalHeight = image_.height() * yMm_;
    const double scale =
        std::min(bounds.width() / physicalWidth, bounds.height() / physicalHeight);
    const int fittedWidth = std::max(1, static_cast<int>(std::floor(physicalWidth * scale)));
    const int fittedHeight = std::max(1, static_cast<int>(std::floor(physicalHeight * scale)));
    const int x = bounds.x() + (bounds.width() - fittedWidth) / 2;
    const int y = bounds.y() + (bounds.height() - fittedHeight) / 2;
    return QRect(x, y, fittedWidth, fittedHeight);
}

QPoint MprViewport::imagePixelAt(const QPoint& widgetPosition) const {
    const QRect target = imageRect();
    if (!hasImage() || !target.contains(widgetPosition)) {
        return QPoint(-1, -1);
    }
    const double u = (widgetPosition.x() - target.left()) / static_cast<double>(target.width());
    const double v = (widgetPosition.y() - target.top()) / static_cast<double>(target.height());
    const int x = std::clamp(static_cast<int>(u * image_.width()), 0, image_.width() - 1);
    const int y = std::clamp(static_cast<int>(v * image_.height()), 0, image_.height() - 1);
    return QPoint(x, y);
}

void MprViewport::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), medforge::ui::tokens::raised);
    painter.setPen(medforge::ui::tokens::textPrimary);
    painter.drawText(QRect(8, 4, width() - 16, 20), Qt::AlignLeft | Qt::AlignVCenter, title_);
    if (!hasImage()) {
        painter.setPen(medforge::ui::tokens::textSecondary);
        painter.drawText(rect().adjusted(12, 28, -12, -12),
                         Qt::AlignCenter | Qt::TextWordWrap,
                         QStringLiteral("No dataset loaded"));
    } else {
        const QRect target = imageRect();
        painter.drawImage(target, image_);
        painter.setPen(medforge::ui::tokens::textSecondary);
        painter.drawText(QRect(2, height() / 2 - 10, 20, 20), Qt::AlignCenter, left_);
        painter.drawText(QRect(width() - 22, height() / 2 - 10, 20, 20), Qt::AlignCenter, right_);
        painter.drawText(QRect(width() / 2 - 12, 24, 24, 16), Qt::AlignCenter, top_);
        painter.drawText(QRect(width() / 2 - 12, height() - 20, 24, 16), Qt::AlignCenter, bottom_);
        if (crosshair_.x() >= 0 && crosshair_.y() >= 0 && crosshair_.x() < image_.width() &&
            crosshair_.y() < image_.height() && target.isValid()) {
            const double px =
                target.left() + (crosshair_.x() + 0.5) * target.width() / image_.width();
            const double py =
                target.top() + (crosshair_.y() + 0.5) * target.height() / image_.height();
            painter.setPen(QPen(medforge::ui::tokens::accent, 1));
            painter.drawLine(QPointF(target.left(), py), QPointF(target.right(), py));
            painter.drawLine(QPointF(px, target.top()), QPointF(px, target.bottom()));
        }
    }
    if (hasFocus()) {
        painter.setPen(QPen(medforge::ui::tokens::accent, 1));
        painter.drawRect(rect().adjusted(0, 0, -1, -1));
    }
}

void MprViewport::wheelEvent(QWheelEvent* event) {
    const int delta = event->angleDelta().y();
    if (delta == 0 || !hasImage()) {
        event->ignore();
        return;
    }
    emit sliceScrolled(delta > 0 ? -1 : 1);
    event->accept();
}

void MprViewport::mousePressEvent(QMouseEvent* event) {
    setFocus(Qt::MouseFocusReason);
    const QPoint pixel = imagePixelAt(event->position().toPoint());
    if (pixel.x() >= 0) {
        emit imageClicked(pixel.x(), pixel.y());
    }
    event->accept();
}

void MprViewport::keyPressEvent(QKeyEvent* event) {
    if (!hasImage()) {
        QWidget::keyPressEvent(event);
        return;
    }
    if (event->key() == Qt::Key_Up || event->key() == Qt::Key_PageUp) {
        emit sliceScrolled(-1);
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Down || event->key() == Qt::Key_PageDown) {
        emit sliceScrolled(1);
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Left) {
        emit imageClicked(crosshair_.x() - 1, crosshair_.y());
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Right) {
        emit imageClicked(crosshair_.x() + 1, crosshair_.y());
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}
