#pragma once
#include <tuple>

namespace passes {
    template <typename... Passes, typename... Args>
    bool apply(Args... args) {
        // Stores the value if we changed something
        //
        bool applied = false;

        // Applying transforms
        //
        const auto do_pass = [&](auto&& pass) -> void {
            applied |= pass.apply(std::forward<Args>(args)...);
        };

        // Creating pass instances and applying them
        //
        std::apply([&](auto&&... pass) { (do_pass(pass), ...); }, std::tuple<Passes...>());

        // Return true if we changed something
        //
        return applied;
    }
} // namespace passes