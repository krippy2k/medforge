#include "medforge/core/CommandHistory.h"

namespace medforge::core {

bool CommandHistory::execute(std::unique_ptr<Command> command) {
    if (command == nullptr || !command->execute()) {
        return false;
    }
    undo_.push_back(std::move(command));
    redo_.clear();
    return true;
}

bool CommandHistory::undo() {
    if (undo_.empty()) {
        return false;
    }
    std::unique_ptr<Command> command = std::move(undo_.back());
    undo_.pop_back();
    if (!command->undo()) {
        undo_.push_back(std::move(command));
        return false;
    }
    redo_.push_back(std::move(command));
    return true;
}

bool CommandHistory::redo() {
    if (redo_.empty()) {
        return false;
    }
    std::unique_ptr<Command> command = std::move(redo_.back());
    redo_.pop_back();
    if (!command->execute()) {
        redo_.push_back(std::move(command));
        return false;
    }
    undo_.push_back(std::move(command));
    return true;
}

bool CommandHistory::canUndo() const {
    return !undo_.empty();
}

bool CommandHistory::canRedo() const {
    return !redo_.empty();
}

std::size_t CommandHistory::undoDepth() const {
    return undo_.size();
}

} // namespace medforge::core
