#include "medforge/core/CommandHistory.h"

#include <QtTest>
#include <memory>

namespace {

class AddCommand : public medforge::core::Command {
public:
    AddCommand(int& target, int delta, bool failExecute = false)
        : target_(target), delta_(delta), failExecute_(failExecute) {}

    const char* name() const override { return "add"; }

    bool execute() override {
        if (failExecute_) {
            return false;
        }
        target_ += delta_;
        return true;
    }

    bool undo() override {
        target_ -= delta_;
        return true;
    }

private:
    int& target_;
    int delta_ = 0;
    bool failExecute_ = false;
};

} // namespace

class CommandHistoryTest : public QObject {
    Q_OBJECT

private slots:
    void undoAndRedoRestoreValue();
    void failedExecuteIsNotRecorded();
    void newExecuteClearsRedo();
};

void CommandHistoryTest::undoAndRedoRestoreValue() {
    int value = 10;
    medforge::core::CommandHistory history;
    QVERIFY(history.execute(std::make_unique<AddCommand>(value, 5)));
    QCOMPARE(value, 15);
    QCOMPARE(history.undoDepth(), std::size_t{1});
    QVERIFY(history.undo());
    QCOMPARE(value, 10);
    QVERIFY(history.canRedo());
    QVERIFY(history.redo());
    QCOMPARE(value, 15);
}

void CommandHistoryTest::failedExecuteIsNotRecorded() {
    int value = 3;
    medforge::core::CommandHistory history;
    QVERIFY(!history.execute(std::make_unique<AddCommand>(value, 1, true)));
    QCOMPARE(value, 3);
    QVERIFY(!history.canUndo());
}

void CommandHistoryTest::newExecuteClearsRedo() {
    int value = 0;
    medforge::core::CommandHistory history;
    QVERIFY(history.execute(std::make_unique<AddCommand>(value, 2)));
    QVERIFY(history.undo());
    QVERIFY(history.canRedo());
    QVERIFY(history.execute(std::make_unique<AddCommand>(value, 4)));
    QVERIFY(!history.canRedo());
    QCOMPARE(value, 4);
}

QTEST_APPLESS_MAIN(CommandHistoryTest)
#include "CommandHistoryTest.moc"
