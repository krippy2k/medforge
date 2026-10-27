#pragma once

#include <QString>
#include <QWidget>

class QLabel;
class QLineEdit;

class NumericEditor : public QWidget {
    Q_OBJECT

public:
    explicit NumericEditor(QWidget* parent = nullptr);

    void setLabel(const QString& label);
    void setUnit(const QString& unit);
    void setRange(double minimum, double maximum);
    void setDecimals(int decimals);

    double value() const;
    void setValue(double value);
    bool isValid() const;
    QString validationError() const;
    void commitText();

signals:
    void valueChanged(double value);
    void validationChanged(bool valid);

private:
    void publishValidation(bool valid, const QString& error);
    void showValue();
    QString rangeMessage() const;
    bool withinRange(double value) const;

    double value_ = 0;
    double minimum_ = 0;
    double maximum_ = 0;
    int decimals_ = 2;
    bool hasRange_ = false;
    QString error_;
    QLabel* label_ = nullptr;
    QLabel* unit_ = nullptr;
    QLabel* errorLabel_ = nullptr;
    QLineEdit* edit_ = nullptr;
};
