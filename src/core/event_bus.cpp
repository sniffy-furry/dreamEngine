#include "dream/core/event_bus.hpp"
#include <algorithm>

namespace dream {

void EventBus::unsubscribe(SubscriptionId id) {
    std::scoped_lock lock(mutex_);
    for (auto& [_, handlers] : handlers_) {
        handlers.erase(
            std::remove_if(handlers.begin(), handlers.end(),
                           [id](const Handler& h) { return h.id == id; }),
            handlers.end());
    }
}

} // namespace dream
