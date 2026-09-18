#include <public/component_signature.h>

#include <algorithm>
#include <functional>

namespace engine::ecs {
ComponentSignature::ComponentSignature(std::initializer_list<ComponentID> signature) {
    _signature = signature;
    std::sort(_signature.begin(), _signature.end(), [](const ComponentID& a, const ComponentID& b) { return a < b; });
    auto it = std::unique(_signature.begin(), _signature.end());
    _signature.erase(it, _signature.end());
}

bool ComponentSignature::has(ComponentID id) const {
    auto it = std::lower_bound(_signature.begin(), _signature.end(), id);
    return it != _signature.end() && *it == id;
}

bool ComponentSignature::add(ComponentID id) {
    auto it = std::lower_bound(_signature.begin(), _signature.end(), id);
    if (it != _signature.end() && *it == id) {
        return false;
    }

    _signature.insert(it, id);
    return true;
}

bool ComponentSignature::remove(ComponentID id) {
    auto it = std::lower_bound(_signature.begin(), _signature.end(), id);
    if (it == _signature.end() || *it != id) {
        return false;
    }

    _signature.erase(it);
    return true;
}

std::size_t ComponentSignature::size() const {
    return _signature.size();
}

const std::vector<ComponentID>& ComponentSignature::get() const {
    return _signature;
}

bool ComponentSignature::operator==(const ComponentSignature& other) const {
    if (size() != other.size()) {
        return false;
    }

    for (std::size_t i = 0; i < size(); i++) {
        if (_signature[i] != other.get()[i]) {
            return false;
        }
    }

    return true;
}

bool ComponentSignature::containsAll(const ComponentSignature& required) const {
    if (required.size() > _signature.size()) {
        return false;
    }

    std::size_t l = 0;
    std::size_t r = 0;

    while (l != _signature.size() && r != required.size()) {
        ComponentID a = _signature[l];
        ComponentID b = required.get()[r];
        if (a == b) {
            l++;
            r++;
        } else if (a < b) {
            l++;
        } else {
            return false;
        }
    }

    return r == required.size();
}

bool ComponentSignature::intersects(const ComponentSignature& other) const {
    std::size_t l = 0;
    std::size_t r = 0;

    while (l != _signature.size() && r != other.size()) {
        ComponentID a = _signature[l];
        ComponentID b = other.get()[r];
        if (a == b) {
            return true;
        } else if (a < b) {
            l++;
        } else {
            r++;
        }
    }

    return false;
}

std::size_t ComponentSignatureHash::operator()(const ComponentSignature& signature) const noexcept {
    std::size_t seed = 0;
    for (const ComponentID& id : signature.get()) {
        const std::size_t idHash = std::hash<ComponentID>{}(id);
        seed ^= idHash + std::size_t{0x9e3779b9U} + (seed << 6U) + (seed >> 2U);
    }
    return seed;
}

} // namespace engine::ecs
