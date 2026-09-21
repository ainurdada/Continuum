#pragma once

#include <type_traits>

namespace editor {

template <typename T> struct CopySnapshotPolicy {
    static_assert(std::is_object_v<T>);
    static_assert(std::is_copy_constructible_v<T>);
    static_assert(std::is_copy_assignable_v<T>);
    
    using Snapshot = T;
    
    static Snapshot capture(const T& value) {
        return value;
    }

    static void restore(T& value, const Snapshot& snapshot) {
        value = snapshot;
    }
};

} // namespace editor