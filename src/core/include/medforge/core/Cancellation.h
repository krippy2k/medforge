#pragma once

#include <atomic>
#include <memory>

namespace medforge::core {

class CancellationToken {
public:
    bool isCancelled() const noexcept;

private:
    friend class CancellationSource;
    explicit CancellationToken(std::shared_ptr<std::atomic<bool>> flag);

    std::shared_ptr<std::atomic<bool>> flag_;
};

class CancellationSource {
public:
    CancellationSource();

    void cancel() noexcept;
    CancellationToken token() const;

private:
    std::shared_ptr<std::atomic<bool>> flag_;
};

} // namespace medforge::core
