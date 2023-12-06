#pragma once
#include "obfuscator/transforms/types.hpp"
#include "util/string_parser.hpp"
#include "util/types.hpp"
#include <any>

namespace obfuscator {
    namespace detail {
        enum e_shared_config_variable_name_index {
            CHANCE = 0,
            REPEAT_TIMES = 1,
        };

        inline std::array kSharedConfigsVariableNames = {"chance", "repeat"};
    } // namespace detail

    /// \brief Configuration class that is used in scheduler for storing transform presets
    struct TransformSharedConfig {
        DEFAULT_DTOR(TransformSharedConfig);
        NON_COPYABLE(TransformSharedConfig);
        TransformSharedConfig(const std::string_view transform_name, const TransformTag transform_tag): name(transform_name), tag(transform_tag) { }

        /// \brief Set how many times we need to re-run the transform
        /// \param times Number of times (1 - 1 time, nullopt - 1 time)
        /// \param override_default Should we override the default value too?
        /// \return Config reference
        TransformSharedConfig& repeat_times(const std::uint8_t times, const bool override_default = false) noexcept {
            repeat_times_ = std::max(static_cast<std::uint8_t>(1), times);
            if (override_default) {
                repeat_times_default_ = repeat_times_;
            }
            return *this;
        }

        /// \brief Get how many times we need to re-run this transform
        /// \return number of times
        [[nodiscard]] std::uint8_t repeat_times() const noexcept {
            return repeat_times_;
        }

        /// \brief Set the transform run chance
        /// \param chance chance (from 0 to 100)%
        /// \param override_default Should we override the default value too?
        /// \return Config reference
        TransformSharedConfig& chance(const std::uint8_t chance, const bool override_default = false) noexcept {
            chance_ = std::clamp(chance, static_cast<std::uint8_t>(0), static_cast<std::uint8_t>(100));
            if (override_default) {
                chance_default_ = chance_;
            }
            return *this;
        }

        /// \brief Get the transform run chance
        /// \return chance (from 0 to 100)%
        [[nodiscard]] std::uint8_t chance() const noexcept {
            return chance_;
        }

        /// \brief Try to load the shared configuration var from string
        /// \param name Var name
        /// \param value Var stringified value
        /// \param override_default Should we override the default value too?
        /// \return true on success, false on failure
        bool try_load(const std::string_view name, const std::string_view value, const bool override_default = false) noexcept {
            static std::unordered_map<std::string, std::function<void()>> callbacks = {};

            /// A little bit of overhead with this once flag, but now the init looks n i c e
            static std::once_flag fl;
            std::call_once(fl, [&]() -> void {
                callbacks[detail::kSharedConfigsVariableNames[detail::CHANCE]] = [&] {
                    chance(util::string::parse_uint8(value), override_default);
                };

                callbacks[detail::kSharedConfigsVariableNames[detail::REPEAT_TIMES]] = [&] {
                    repeat_times(util::string::parse_uint8(value), override_default);
                };
            });

            /// Try to find the loader, and load if found
            if (const auto it = callbacks.find(name.data()); it != std::end(callbacks)) {
                it->second();
                return true;
            }

            /// Not found
            return false;
        }

        /// \brief Stringify var value by its name
        /// \param var_name variable name
        /// \return stringified value
        [[nodiscard]] std::string stringify_var(const std::string_view var_name) const noexcept {
            if (var_name == detail::kSharedConfigsVariableNames[detail::CHANCE]) {
                return std::to_string(chance_);
            }
            if (var_name == detail::kSharedConfigsVariableNames[detail::REPEAT_TIMES]) {
                return std::to_string(repeat_times());
            }
            return "unknown var"; // maybe we should throw an exception?
        }

        /// \brief Reset the shared config state
        void reset() {
            repeat_times_ = repeat_times_default_;
            chance_ = chance_default_;
        }

        /// \brief Transform human-readable name
        const std::string name;
        /// \brief Transform internal identifier
        const TransformTag tag;

    private:
        /// \brief How many times do we need to re-run this transform
        /// \note If set to 1, the transform would be executed only 1 time
        std::uint8_t repeat_times_default_ = 1;
        std::uint8_t repeat_times_ = repeat_times_default_;

        /// \brief Transform run chance in percents
        std::uint8_t chance_default_ = 30; // (from 0 to 100)%
        std::uint8_t chance_ = chance_default_;
    };

    /// \brief Transform configuration storage
    class TransformSharedConfigStorage : public types::Singleton<TransformSharedConfigStorage> {
    public:
        DEFAULT_CTOR_DTOR(TransformSharedConfigStorage);
        NON_COPYABLE(TransformSharedConfigStorage);

        /// \brief Get transform config using the transform tag
        [[nodiscard]] TransformSharedConfig& get_for(const TransformTag tag, const std::optional<std::string_view>& name = std::nullopt) {
            /// If already cached
            if (const auto it = configurations_.find(tag); it != std::end(configurations_)) {
                return it->second;
            }

            /// Otherwise create new one
            configurations_.emplace(std::piecewise_construct, std::make_tuple(tag), std::make_tuple(*name, tag));
            return configurations_.at(tag);
        }

        /// \brief Get transform config using the transform type
        template <template <pe::any_image_t> class Ty>
        [[nodiscard]] TransformSharedConfig& get_for() {
            return get_for(get_transform_tag<Ty>(), get_transform_name<Ty>());
        }

        /// \brief Get transform config using its name
        [[nodiscard]] TransformSharedConfig& get_for_name(const std::string_view name) {
            const auto it = std::ranges::find_if(configurations_, [name](const auto& p) -> bool { return p.second.name == name; });

            if (it == std::end(configurations_)) {
                throw std::runtime_error(std::format("configs: Unable to find configuration for transform {}", name));
            }

            return it->second;
        }

    private:
        /// \brief Configuration storage
        std::unordered_map<TransformTag, TransformSharedConfig> configurations_ = {};
    };

    /// \brief Transform configuration storage
    class TransformConfig {
    public:
        DEFAULT_CTOR_DTOR(TransformConfig);
        NON_COPYABLE(TransformConfig);
        using Index = std::size_t;

        class Var {
        public:
            DEFAULT_CTOR_DTOR(Var);
            DEFAULT_COPY(Var);

            enum Type {
                GLOBAL = 0, // one option for all functions
                PER_FUNCTION, // sets per function
            };

            /// \brief Get the variable as desired type
            /// \tparam Ty type
            /// \return Value
            template <typename Ty>
            [[nodiscard]] Ty value() const {
                assert(value_.has_value());
                return std::any_cast<Ty>(value_);
            }

            /// \brief Get the variable, or default value if its value is unset
            /// \tparam Ty type
            /// \param default_value default value
            /// \return value
            template <typename Ty>
            [[nodiscard]] Ty value_or(const Ty default_value) const {
                if (!value_.has_value()) {
                    return default_value;
                }
                return std::any_cast<Ty>(value_);
            }

            /// \brief Set the variable value
            /// \tparam Ty type
            /// \param value value that it should set
            template <typename Ty>
            void set(const Ty value) {
                if (!value_.has_value()) {
                    default_value_.emplace<Ty>(value);
                }
                value_.emplace<Ty>(value);
            }

            /// \brief Set new value parsed from string
            /// \param value string that contain the new value
            void parse_value(const std::string_view value) {
                util::string::parse_to_any(value_, value);
            }

            /// \brief Verify that this variable has a value set
            /// \return true/false
            [[nodiscard]] bool is_set() const {
                return value_.has_value();
            }

            /// \brief Get the var type
            /// \return Type
            [[nodiscard]] Type var_type() const {
                return type_;
            }

            /// \brief Get var name
            /// \return Name
            [[nodiscard]] std::string name() const {
                return name_;
            }

            /// \brief Get var short description
            /// \return Name
            [[nodiscard]] std::optional<std::string> short_description() const {
                return short_description_;
            }

            /// \brief Set short description (1 line max)
            /// \param short_description description value
            void short_description(const std::string& short_description) {
                short_description_ = short_description;
            }

            /// \brief Is var required
            /// \return bool
            [[nodiscard]] bool required() const {
                return required_;
            }

            /// \brief Serialize current value as string
            /// \return string
            [[nodiscard]] std::string serialize() const {
                return util::string::serialize_any(value_);
            }

            /// \brief Set the variable info
            /// \param name variable name
            /// \param is_required is required
            /// \param type variable type, either global or per function
            void set_info(const std::string& name, const bool is_required, const Type type) {
                name_ = name;
                required_ = is_required;
                type_ = type;
            }

            /// \brief Reset variable to its default value
            void reset() {
                value_ = default_value_;
            }

        private:
            /// \brief Value holder
            std::any value_ = {};
            /// \brief First value holder
            std::any default_value_ = {};
            /// \brief Var name
            std::string name_ = "";
            /// \brief Short description (1 line max)
            std::optional<std::string> short_description_ = "";
            /// \brief Is var required to set by user
            bool required_ = false;
            /// \brief Variable type, either global or per function
            Type type_ = GLOBAL;
        };

        /// \brief Create a new var with default value
        /// \tparam Ty value type
        /// \param index variable unique index
        /// \param name variable name
        /// \param is_required is required
        /// \param type Variable type, either global or per function
        /// \param default_value variable default value
        /// \return Var reference
        template <typename Ty>
        Var& new_var(const Index index, const std::string& name, const bool is_required, const Var::Type type, const Ty default_value) {
            variables_[index].set_info(name, is_required, type);
            auto& result = variables_.at(index);
            result.set(default_value);
            return result;
        }

        /// \brief Get variable object
        /// \param index variable index
        /// \return var object reference
        [[nodiscard]] const Var& get_var(const Index index) const {
            if (const auto iter = variables_.find(index); iter != std::end(variables_)) {
                return iter->second;
            }

            throw std::runtime_error(std::format("configs: unable to find var {}", index));
        }

        /// \brief Get variable object by name
        /// \param name Var name
        /// \return var object reference
        [[nodiscard]] Var& get_var_by_name(const std::string_view name) {
            if (const auto iter = std::ranges::find_if(variables_, [name](const auto& p) -> bool { return p.second.name() == name; });
                iter != std::end(variables_)) {
                return iter->second;
            }

            throw std::runtime_error(std::format("configs: unable to find var {}", name));
        }

        /// \brief Get variable value
        /// \tparam Ty value type
        /// \param index var index
        /// \return value
        template <typename Ty>
        [[nodiscard]] Ty get_value(const Index index) const {
            return get_var(index).value<Ty>();
        }

        /// \brief Reset all config vars to their default values
        /// \param type type of vars that it should reset
        void reset_vars(const Var::Type type) {
            for (auto& [_, var] : variables_) {
                if (var.var_type() != type) {
                    continue;
                }

                var.reset();
            }
        }

        /// \brief Iterate over the all transform vars
        /// \param callback foreach callback
        void iter_vars(const std::function<void(Var&)>& callback) {
            std::ranges::for_each(variables_, [callback](auto&& p) -> void { callback(p.second); });
        }

    private:
        /// \brief A map that stores all the variables with their indices
        std::unordered_map<Index, Var> variables_ = {};
    };
} // namespace obfuscator
