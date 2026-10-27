#include "medforge/core/Cancellation.h"

namespace medforge::core {

CancellationToken::CancellationToken(std::shared_ptr<std::atomic<bool>> flag)
    : flag_(std::move(flag)) {}

bool CancellationToken::isCancelled() const noexcept {
    return flag_ != nullptr && flag_->load(std::memory_order_acquire);
}

CancellationSource::CancellationSource() : flag_(std::make_shared<std::atomic<bool>>(false)) {}

void CancellationSource::cancel() noexcept {
    if (flag_ != nullptr) {
        flag_->store(true, std::memory_order_release);
    }
}

CancellationToken CancellationSource::token() const {
    return CancellationToken{flag_};
}

} // namespace medforge::core
