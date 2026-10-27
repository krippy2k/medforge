#include "medforge/ui/NumericEditor.h"

#include <QLabel>
#include <QLineEdit>
#include <QSignalSpy>
#include <QtTest>

class NumericEditorTest : public QObject {
    Q_OBJECT

private slots:
    void showsLabelAndUnit();
    void acceptsInRangeValue();
    void rejectsOutOfRangeAndKeepsPreviousValue();
    void rejectsEmptyText();
    void disabledEditorDoesNotAcceptEdits();
};

void NumericEditorTest::showsLabelAndUnit() {
    NumericEditor editor;
    editor.setLabel(QStringLiteral("Length"));
    editor.setUnit(QStringLiteral("mm"));
    const auto* unit = editor.findChild<QLabel*>(QStringLiteral("numericUnit"));
    QVERIFY(unit != nullptr);
    QCOMPARE(unit->text(), QStringLiteral("mm"));
    const auto* edit = editor.findChild<QLineEdit*>(QStringLiteral("numericValue"));
    QVERIFY(edit != nullptr);
    QCOMPARE(edit->accessibleName(), QStringLiteral("Length"));
}

void NumericEditorTest::acceptsInRangeValue() {
    NumericEditor editor;
    editor.setRange(0, 10);
    editor.setDecimals(1);
    auto* edit = editor.findChild<QLineEdit*>(QStringLiteral("numericValue"));
    edit->setText(QStringLiteral("4.5"));
    editor.commitText();
    QVERIFY(editor.isValid());
    QVERIFY(editor.validationError().isEmpty());
    QCOMPARE(editor.value(), 4.5);
}

void NumericEditorTest::rejectsOutOfRangeAndKeepsPreviousValue() {
    NumericEditor editor;
    editor.setRange(0, 10);
    editor.setUnit(QStringLiteral("mm"));
    editor.setValue(4);
    QSignalSpy values(&editor, &NumericEditor::valueChanged);
    auto* edit = editor.findChild<QLineEdit*>(QStringLiteral("numericValue"));
    edit->setText(QStringLiteral("12"));
    editor.commitText();
    QVERIFY(!editor.isValid());
    QVERIFY(editor.validationError().contains(QStringLiteral("10")));
    QVERIFY(editor.validationError().contains(QStringLiteral("mm")));
    QCOMPARE(editor.value(), 4.0);
    QCOMPARE(values.count(), 0);
}

void NumericEditorTest::rejectsEmptyText() {
    NumericEditor editor;
    editor.setRange(0, 10);
    editor.setValue(4);
    auto* edit = editor.findChild<QLineEdit*>(QStringLiteral("numericValue"));
    edit->setText(QString());
    editor.commitText();
    QVERIFY(!editor.isValid());
    QVERIFY(!editor.validationError().isEmpty());
    QCOMPARE(editor.value(), 4.0);
}

void NumericEditorTest::disabledEditorDoesNotAcceptEdits() {
    NumericEditor editor;
    editor.setRange(0, 10);
    editor.setValue(4);
    editor.setEnabled(false);
    auto* edit = editor.findChild<QLineEdit*>(QStringLiteral("numericValue"));
    QVERIFY(edit != nullptr);
    QVERIFY(!edit->isEnabled());
    edit->setText(QStringLiteral("9"));
    editor.commitText();
    QCOMPARE(editor.value(), 4.0);
    QVERIFY(editor.isValid());
}

QTEST_MAIN(NumericEditorTest)
#include "NumericEditorTest.moc"
