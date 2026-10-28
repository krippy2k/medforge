#pragma once

#include <QImage>
#include <QPoint>
#include <QString>
#include <QVector>
#include <QWidget>

struct PlaneAnnotation {
    QPoint pixel;
    QString label;
    bool selected = false;
};

struct PlaneLink {
    QPoint from;
    QPoint to;
};

class MprViewport : public QWidget {
    Q_OBJECT

public:
    explicit MprViewport(const QString& title, QWidget* parent = nullptr);

    void setImage(const QImage& image);
    void setMillimetersPerPixel(double xMm, double yMm);
    void clearImage();
    void setCrosshair(const QPoint& pixel);
    void setAnnotations(const QVector<PlaneAnnotation>& markers, const QVector<PlaneLink>& links);
    void setOrientation(const QString& left,
                        const QString& right,
                        const QString& top,
                        const QString& bottom);

    bool hasImage() const;
    QSize imagePixelSize() const;
    QRect imageBounds() const;
    QPoint crosshair() const;
    int grayAt(int x, int y) const;
    QString planeTitle() const;
    QString leftLabel() const;
    QString rightLabel() const;
    QString topLabel() const;
    QString bottomLabel() const;

signals:
    void sliceScrolled(int steps);
    void imageClicked(int x, int y);
    void imageDragged(int x, int y);

protected:
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    QRect imageRect() const;
    QPoint imagePixelAt(const QPoint& widgetPosition) const;

    QString title_;
    QImage image_;
    QPoint crosshair_;
    QVector<PlaneAnnotation> markers_;
    QVector<PlaneLink> links_;
    double xMm_ = 1;
    double yMm_ = 1;
    QString left_ = QStringLiteral("R");
    QString right_ = QStringLiteral("L");
    QString top_ = QStringLiteral("A");
    QString bottom_ = QStringLiteral("P");
};
