#pragma once
#include "obfuscator/transforms/configs.hpp"
#include "obfuscator/transforms/transform.hpp"
#include "obfuscator/transforms/types.hpp"

namespace obfuscator {
    /// \brief A container that stores transforms and their schedule state
    /// \tparam Img PE Image type, either x64 or x86
    class TransformContainer {
    public:
        DEFAULT_CT_CTOR_DTOR(TransformContainer);
        NON_COPYABLE(TransformContainer);
        using TransformPtr = std::unique_ptr<Transform>;
        using PairPtr = std::pair<TransformTag, Transform*>;

        /// \brief Register a transform under its tag
        /// \tparam Ty Transform type
        template <typename Ty>
        TransformSharedConfig& register_transform() {
            /// Init transform
            const auto tag = get_transform_tag<Ty>();
            auto instance = std::make_unique<Ty>();
            instance->init();

            /// Save transform
            transforms[tag] = std::move(instance);

            /// Init the config and return it
            return TransformSharedConfigStorage::get().get_for<Ty>();
        }

        /// \brief Enable desired transform
        /// \param tag transform tag
        /// \return Config reference
        TransformSharedConfig& enable_transform(const TransformTag tag) {
            if (std::ranges::find(enabled, tag) == std::end(enabled)) {
                enabled.emplace_back(tag);
            }
            return TransformSharedConfigStorage::get().get_for(tag);
        }

        /// \brief Select transforms by their tags
        [[nodiscard]] auto select_transforms(const std::vector<TransformTag>& tags,
                                             const std::optional<TransformFeaturesSet::Index> feature_filter = std::nullopt) {
            std::vector<std::pair<TransformTag, Transform*>> result = {};

            for (const auto tag : tags) {
                const auto it = transforms.find(tag);
                if (it == std::end(transforms)) {
                    throw std::runtime_error(std::format("scheduler: unable to find transform with tag {:#x}", tag));
                }

                if (feature_filter.has_value() && !it->second->feature(feature_filter.value())) {
                    continue;
                }

                result.emplace_back(std::make_pair(tag, it->second.get()));
            }

            return result;
        }

        /// \brief Iterate over the enabled transforms using callback
        /// \param callback callback that should be invoked for every entry
        void iter_enabled_transforms(const std::function<void(Transform*)>& callback) {
            std::ranges::for_each(enabled, [this, &callback](const TransformTag tag) -> void { callback(transforms.at(tag).get()); });
        }

        /// \brief Get a std::views iterator for transforms
        auto transforms_iterator() {
            return std::views::all(transforms) | std::views::filter([this](const auto& value) -> bool {
                       return std::ranges::find(this->enabled, value.first) != std::end(this->enabled);
                   }) |
                   std::views::keys | std::views::transform([](const auto& value) -> Transform* { return value.get(); });
        }

        /// \brief Get a std::views iterator for transforms
        auto transforms_iterator() const {
            return std::views::all(transforms) | std::views::filter([this](const auto& value) -> bool {
                       return std::ranges::find(this->enabled, value.first) != std::end(this->enabled);
                   }) |
                   std::views::keys | std::views::transform([](const auto& value) -> Transform* { return value.get(); });
        }

        /// \brief Transforms iterator begin for the ranged loops
        /// \return iterator
        [[nodiscard]] auto begin() {
            return transforms_iterator().begin();
        }

        /// \brief Transforms const iterator begin for the ranged loops
        /// \return iterator
        [[nodiscard]] auto begin() const {
            return transforms_iterator().begin();
        }

        /// \brief Transforms iterator end for the ranged loops
        /// \return iterator
        [[nodiscard]] auto end() {
            return transforms_iterator().end();
        }

        /// \brief Transforms const iterator end for the ranged loops
        /// \return iterator
        [[nodiscard]] auto end() const {
            return transforms_iterator().end();
        }

        /// \brief A map that stores enabled transforms
        std::vector<TransformTag> enabled;

        /// \brief A map that stores transforms under their tags
        std::unordered_map<TransformTag, TransformPtr> transforms;
    };

    /// \brief Transform scheduler that stores all the transforms and their schedule state
    class TransformScheduler : public types::Singleton<TransformScheduler> {
    public:
        /// \brief Register a desired transform under the transform tag for **both** x64 and x86 architectures
        /// \tparam Ty Transform type
        template <typename Ty>
        TransformSharedConfig& register_transform() {
            return container.register_transform<Ty>();
        }

        /// \brief Get the total number of enabled transforms
        /// \return
        [[nodiscard]] std::size_t enabled_count() const {
            return container.enabled.size();
        }

        /// \brief Enable transform on all architectures
        /// \param tag transform tag
        void enable_transform(const TransformTag tag) {
            container.enable_transform(tag);
        }

        /// \fixme @es3n1n: this should be removed, just container with no scheduler
        TransformContainer container;
    };

    /// \brief Scheduler initialization routine
    void startup_scheduler();
} // namespace obfuscator
