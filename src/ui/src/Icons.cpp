#include "medforge/ui/Icons.h"

#include "medforge/ui/DesignTokens.h"

#include <QPainter>
#include <QPen>
#include <QPixmap>

namespace medforge::ui::icons {
namespace {

QIcon strokeIcon(void (*draw)(QPainter&)) {
    QIcon icon;
    for (const qreal dpr : {1.0, 2.0}) {
        const int pixels = qRound(16.0 * dpr);
        QPixmap pixmap(pixels, pixels);
        pixmap.setDevicePixelRatio(dpr);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        QPen pen(tokens::textPrimary, 1.5);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        draw(painter);
        icon.addPixmap(pixmap);
    }
    return icon;
}

} // namespace

QIcon search() {
    return strokeIcon([](QPainter& painter) {
        painter.drawEllipse(QRectF(2.5, 2.5, 7.0, 7.0));
        painter.drawLine(QPointF(9.2, 9.2), QPointF(13.2, 13.2));
    });
}

QIcon clear() {
    return strokeIcon([](QPainter& painter) {
        painter.drawLine(QPointF(4, 4), QPointF(12, 12));
        painter.drawLine(QPointF(12, 4), QPointF(4, 12));
    });
}

QIcon folder() {
    return strokeIcon([](QPainter& painter) {
        painter.drawRect(QRectF(2, 5, 12, 8));
        painter.drawLine(QPointF(2, 5), QPointF(2, 4));
        painter.drawLine(QPointF(2, 4), QPointF(7, 4));
        painter.drawLine(QPointF(7, 4), QPointF(8, 5));
    });
}

QIcon check() {
    return strokeIcon([](QPainter& painter) {
        painter.drawLine(QPointF(3, 8.5), QPointF(6.5, 12));
        painter.drawLine(QPointF(6.5, 12), QPointF(13, 4));
    });
}

QIcon chevronDown() {
    return strokeIcon([](QPainter& painter) {
        painter.drawLine(QPointF(3.5, 6), QPointF(8, 10.5));
        painter.drawLine(QPointF(8, 10.5), QPointF(12.5, 6));
    });
}

QIcon chevronRight() {
    return strokeIcon([](QPainter& painter) {
        painter.drawLine(QPointF(6, 3.5), QPointF(10.5, 8));
        painter.drawLine(QPointF(10.5, 8), QPointF(6, 12.5));
    });
}

QIcon warning() {
    return strokeIcon([](QPainter& painter) {
        painter.drawLine(QPointF(8, 2.5), QPointF(14, 13));
        painter.drawLine(QPointF(14, 13), QPointF(2, 13));
        painter.drawLine(QPointF(2, 13), QPointF(8, 2.5));
        painter.drawLine(QPointF(8, 6.5), QPointF(8, 9.5));
    });
}

QIcon info() {
    return strokeIcon([](QPainter& painter) {
        painter.drawEllipse(QRectF(2.5, 2.5, 11, 11));
        painter.drawLine(QPointF(8, 7.2), QPointF(8, 11.2));
        painter.drawPoint(QPointF(8, 5.2));
    });
}

} // namespace medforge::ui::icons
