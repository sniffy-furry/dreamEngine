#pragma once
#include <memory>
#include <string_view>
#include <typeindex>
#include <unordered_map>

namespace dream {

class Registry final {
public:
    template<class T>
    void provide(std::shared_ptr<T> service) {
        services_[std::type_index(typeid(T))] = std::move(service);
    }

    template<class T>
    std::shared_ptr<T> get() const {
        auto it = services_.find(std::type_index(typeid(T)));
        if (it == services_.end()) return {};
        return std::static_pointer_cast<T>(it->second);
    }

private:
    std::unordered_map<std::type_index, std::shared_ptr<void>> services_;
};

} // namespace dream
