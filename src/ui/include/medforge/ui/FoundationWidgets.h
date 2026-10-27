#pragma once

#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QWidget>

class QHBoxLayout;
class QKeyEvent;
class QMouseEvent;
class QProgressBar;
class QPushButton;
class QToolButton;
class QVBoxLayout;

class SearchField : public QLineEdit {
public:
    explicit SearchField(QWidget* parent = nullptr);
};

class BrowserRow : public QWidget {
    Q_OBJECT

public:
    explicit BrowserRow(const QString& label, QWidget* parent = nullptr);

    void setDepth(int depth);
    void setCountText(const QString& text);
    void setSelected(bool selected);
    void setExpanded(bool expanded);
    bool isSelected() const;
    bool isExpanded() const;
    QString label() const;

signals:
    void activated();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    QString label_;
    bool selected_ = false;
    bool expanded_ = true;
    QWidget* indent_ = nullptr;
    QToolButton* toggle_ = nullptr;
    QLabel* labelWidget_ = nullptr;
    QLabel* countWidget_ = nullptr;
};

class PropertyGroup : public QWidget {
public:
    explicit PropertyGroup(const QString& title, QWidget* parent = nullptr);

    void addRow(QWidget* row);
    void setExpanded(bool expanded);
    bool isExpanded() const;

private:
    bool expanded_ = true;
    QToolButton* toggle_ = nullptr;
    QWidget* content_ = nullptr;
    QVBoxLayout* contentLayout_ = nullptr;
};

class DockHeader : public QWidget {
public:
    explicit DockHeader(const QString& title, QWidget* parent = nullptr);
    QToolButton* addToolButton(const QIcon& icon, const QString& accessibleName);

private:
    QHBoxLayout* layout_ = nullptr;
};

class TaskProgress : public QWidget {
    Q_OBJECT

public:
    explicit TaskProgress(QWidget* parent = nullptr);

    void setRunning(bool running);
    void setPercent(int percent);
    int percent() const;
    bool isRunning() const;

signals:
    void cancelled();

private:
    QProgressBar* bar_ = nullptr;
    QPushButton* cancel_ = nullptr;
    int percent_ = 0;
    bool running_ = false;
};

class Banner : public QWidget {
    Q_OBJECT

public:
    enum class Severity { Notice, Warning, Error, Success };

    explicit Banner(QWidget* parent = nullptr);

    void setMessage(Severity severity, const QString& message);
    void setDismissible(bool dismissible);
    QString severityText() const;
    QString message() const;
    Severity severity() const;

signals:
    void dismissed();

private:
    Severity severity_ = Severity::Notice;
    QLabel* severityLabel_ = nullptr;
    QLabel* messageLabel_ = nullptr;
    QToolButton* dismiss_ = nullptr;
};

class EmptyState : public QWidget {
    Q_OBJECT

public:
    EmptyState(const QString& title,
               const QString& body,
               const QString& actionLabel,
               QWidget* parent = nullptr);

signals:
    void actionTriggered();
};

class ValidationMessage : public QLabel {
public:
    explicit ValidationMessage(QWidget* parent = nullptr);
    void setError(const QString& fieldName, const QString& message);
};
