#pragma once
#include "obfuscator/transforms/configs.hpp"
#include "obfuscator/transforms/scheduler.hpp"
#include "obfuscator/transforms/transform.hpp"

namespace obfuscator::config_merger {
    namespace detail {
        /// \brief Util function to get all required vars for a transform
        /// \tparam Img X64 or X86 image
        /// \param transform Transform pointer
        /// \param type Var type
        /// \return vector of names
        template <pe::any_image_t Img>
        std::vector<std::string> get_all_required_vars_for(Transform<Img>* transform, const TransformConfig::Var::Type type) {
            std::vector<std::string> required_var_names = {};

            transform->iter_vars([&required_var_names, type](const TransformConfig::Var& var) -> void {
                if (var.var_type() != type) {
                    return;
                }
                if (!var.required()) {
                    return;
                }
                required_var_names.emplace_back(var.name());
            });

            return required_var_names;
        }

        /// \brief An util that applies desired config
        /// \tparam Img X64 or X86 image
        /// \param transform Transform pointer
        /// \param type var type
        /// \param values config values
        /// \param shared_config shared config reference
        template <pe::any_image_t Img>
        void apply_vars(Transform<Img>* transform, const TransformConfig::Var::Type type, const std::unordered_map<std::string, std::string>& values,
                        TransformSharedConfig& shared_config) {
            /// Collect all required vars
            auto required_var_names = get_all_required_vars_for(transform, type);

            /// Reset shared config state if we're parsing PER_FUNCTION stuff
            if (type == TransformConfig::Var::Type::PER_FUNCTION) {
                shared_config.reset();
            }

            /// Iterate over the vars
            for (auto&& [var_name, var_s_value] : values) {
                /// Try to load to shared config first
                if (shared_config.try_load(var_name, var_s_value, type == TransformConfig::Var::Type::GLOBAL)) {
                    continue;
                }

                /// Get the var
                auto& var = transform->get_var_by_name(var_name);

                /// Assert var type, we are only dealing with global ones here
                if (var.var_type() != type) {
                    throw std::runtime_error(std::format("apply_global_vars: unable to set the {} value", var_name));
                }

                /// Parse the value
                var.parse_value(var_s_value);

                /// Mark as set if this var was required
                std::erase_if(required_var_names, [&var](const auto& name) -> bool {
                    return var.name() == name; //
                });
            }

            /// Throw an error if some vars were unset
            if (!required_var_names.empty()) {
                throw std::runtime_error(std::format("apply_config: unable to find required {} value", required_var_names.at(0)));
            }
        }
    } // namespace detail

    /// \brief Apply transform global vars
    /// \tparam Img X64 or X86 image
    /// \param config config reference
    template <pe::any_image_t Img>
    void apply_global_vars(config_parser::Config& config) {
        /// Get the scheduler
        auto& scheduler = TransformScheduler::get().for_arch<Img>();

        /// Iterate over the global defined vars for the transform
        for (auto& [tag, values] : config.global_transforms_config()) {
            /// Get the transform, its shared config
            auto& transform = scheduler.transforms.at(tag);
            auto& shared_config = TransformSharedConfigStorage::get().get_for(tag);

            /// Apply vars
            detail::apply_vars(transform.get(), TransformConfig::Var::Type::GLOBAL, values, shared_config);
        }
    }

    /// \brief Apply user-defined configuration for the transform
    /// \tparam Img X64 or X86 image
    /// \param transform_config user-defined options
    template <pe::any_image_t Img>
    void apply_config(const config_parser::transform_configuration_t& transform_config) {
        /// Get all the needed stuff
        auto& scheduler = TransformScheduler::get().for_arch<Img>();
        auto& transform = scheduler.transforms.at(transform_config.tag);
        auto& shared_config = TransformSharedConfigStorage::get().get_for(transform_config.tag);

        /// Reset all PER_FUNCTION vars
        transform->reset_config(TransformConfig::Var::PER_FUNCTION);

        /// Apply vars
        detail::apply_vars(transform.get(), TransformConfig::Var::Type::PER_FUNCTION, transform_config.values, shared_config);
    }
} // namespace obfuscator::config_merger
