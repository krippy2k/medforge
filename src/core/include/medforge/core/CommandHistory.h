#pragma once

#include <cstddef>
#include <memory>
#include <vector>

namespace medforge::core {

class Command {
public:
    virtual ~Command() = default;
    virtual const char* name() const = 0;
    virtual bool execute() = 0;
    virtual bool undo() = 0;
};

class CommandHistory {
public:
    bool execute(std::unique_ptr<Command> command);
    bool undo();
    bool redo();
    bool canUndo() const;
    bool canRedo() const;
    std::size_t undoDepth() const;

private:
    std::vector<std::unique_ptr<Command>> undo_;
    std::vector<std::unique_ptr<Command>> redo_;
};

} // namespace medforge::core
