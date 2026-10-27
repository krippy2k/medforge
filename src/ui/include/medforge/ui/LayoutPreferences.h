#pragma once

#include <QByteArray>
#include <QString>

class LayoutPreferences {
public:
    explicit LayoutPreferences(const QString& filePath);

    QString path() const;
    void save(const QByteArray& state, const QByteArray& geometry);
    bool load(QByteArray* state, QByteArray* geometry) const;
    void reset();
    bool hasSavedLayout() const;

private:
    QString path_;
};
