#pragma once
#include <cstdint>
#include <functional>
#include <mutex>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace dream {

class EventBus final {
public:
    using SubscriptionId = std::uint64_t;

    template<class Event>
    SubscriptionId subscribe(std::function<void(const Event&)> callback) {
        const auto type = std::type_index(typeid(Event));
        std::scoped_lock lock(mutex_);
        const SubscriptionId id = ++next_id_;
        handlers_[type].push_back(Handler{
            id,
            [fn = std::move(callback)](const void* e) {
                fn(*static_cast<const Event*>(e));
            }
        });
        return id;
    }

    template<class Event>
    void publish(const Event& event) {
        const auto type = std::type_index(typeid(Event));
        std::vector<Handler> snapshot;
        {
            std::scoped_lock lock(mutex_);
            auto it = handlers_.find(type);
            if (it == handlers_.end()) return;
            snapshot = it->second;
        }
        for (const auto& h : snapshot) h.callback(&event);
    }

    void unsubscribe(SubscriptionId id);

private:
    struct Handler {
        SubscriptionId id;
        std::function<void(const void*)> callback;
    };

    std::mutex mutex_;
    SubscriptionId next_id_ = 0;
    std::unordered_map<std::type_index, std::vector<Handler>> handlers_;
};

} // namespace dream
