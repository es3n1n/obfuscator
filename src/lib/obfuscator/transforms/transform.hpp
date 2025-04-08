#pragma once
#include "obfuscator/function.hpp"

namespace obfuscator {
    /// \brief Feature set represents what features does this transform support.
    /// Can contain only bool values by design.
    class TransformFeaturesSet {
    public:
        DEFAULT_CTOR_DTOR(TransformFeaturesSet);
        DEFAULT_COPY(TransformFeaturesSet);

        /// \brief Available features
        enum Index : std::uint8_t {
            HAS_FUNCTION_TRANSFORM = 0,
            HAS_BB_TRANSFORM,
            HAS_NODE_TRANSFORM,
            HAS_INSN_TRANSFORM,
            /// \todo @es3n1n: *_has_chance_check
        };

        /// \brief Get feature by its index
        /// \param feature Feature index
        /// \return reference to value
        bool& get(const Index feature) {
            return values_[feature];
        }

    private:
        /// \brief Features storage, by default it would initialize values to false
        std::unordered_map<Index, bool> values_;
    };

    /// \brief Transform context that gets passed to the transform callback
    class TransformContext {
    public:
        DEFAULT_DTOR(TransformContext);
        NON_COPYABLE(TransformContext);
        explicit TransformContext(TransformSharedConfig& shared_config_value): shared_config(shared_config_value) { }

        /// \brief Shared config reference
        TransformSharedConfig& shared_config;

        /// \brief An option that could be set to true in order to force the obfuscator to re-run
        /// the transform, ignoring the `repeat_times` from its config.
        bool rerun_me = false;
    };

    /// \brief Obfuscation transform
    /// \tparam Img PE Image type, either x64 or x86
    template <pe::any_image_t Img>
    class Transform {
    public:
        DEFAULT_CT_CTOR(Transform);
        NON_COPYABLE(Transform);
        virtual ~Transform() = default;

        /// \brief Callback that initializes `features_set_`
        virtual void init_features() = 0;

        /// \brief Optional callback that initializes config variables
        virtual void init_config() { }

        /// \brief Transform routine
        /// \param ctx Transform context
        /// \param function Routine that it should transform
        virtual void run_on_function(TransformContext& ctx, Function<Img>* function) = 0;

        /// \brief Transform basic block
        /// \param ctx Transform context
        /// \param function Routine that it should transform
        /// \param basic_block Basic block that it should transform
        virtual void run_on_bb(TransformContext& ctx, Function<Img>* function, analysis::bb_t* basic_block) = 0;

        /// \brief Transform zasm node
        /// \param ctx Transform context
        /// \param function Routine that it should transform
        /// \param node Node that it should transform
        virtual void run_on_node(TransformContext& ctx, Function<Img>* function, zasm::Node* node) = 0;

        /// \brief Transform analysis insn
        /// \param ctx Transform context
        /// \param function Routine that it should transform
        /// \param insn Instruction that it should transform
        virtual void run_on_insn(TransformContext& ctx, Function<Img>* function, analysis::insn_t* insn) = 0;

        /// \brief General purpose initializer
        void init() {
            this->init_features();
            this->init_config();
        }

        /// \brief `has_function_transform` getter
        [[nodiscard]] bool feature(const TransformFeaturesSet::Index index) noexcept {
            return features_set_.get(index);
        }

        /// \brief `has_node_transform` setter
        /// \param index feature index
        /// \param value new value
        void feature(const TransformFeaturesSet::Index index, const bool value) {
            features_set_.get(index) = value;
        }

        /// \brief Create a new config var with default value
        /// \tparam Ty value type
        /// \param index variable unique index
        /// \param name variable name
        /// \param is_required is required
        /// \param type Variable type, either global or per function
        /// \param default_value variable default value
        /// \return Var reference
        template <typename Ty>
        TransformConfig::Var& new_var(const TransformConfig::Index index, const std::string& name, const bool is_required,
                                      const TransformConfig::Var::Type type, const Ty default_value) {
            return config_.new_var<Ty>(index, name, is_required, type, default_value);
        }

        /// \brief Get config variable object
        /// \param index variable index
        /// \return var object reference
        [[nodiscard]] const TransformConfig::Var& get_var(const TransformConfig::Index index) const {
            return config_.get_var(index);
        }

        /// \brief Get config variable value
        /// \tparam Ty value type
        /// \param index var index
        /// \return value
        template <typename Ty>
        [[nodiscard]] Ty get_var_value(const TransformConfig::Index index) const {
            return config_.get_value<Ty>(index);
        }

        /// \brief Reset all config vars to their default values
        /// \param type type of vars that it should reset
        void reset_config(const TransformConfig::Var::Type type) {
            config_.reset_vars(type);
        }

        /// \brief Get variable object by name
        /// \param name Var name
        /// \return var object reference
        [[nodiscard]] TransformConfig::Var& get_var_by_name(const std::string_view name) {
            return config_.get_var_by_name(name);
        }

        void iter_vars(const std::function<void(TransformConfig::Var&)>& callback) {
            config_.iter_vars(callback);
        }

    private:
        /// \brief Features set
        TransformFeaturesSet features_set_;
        /// \brief Config vars storage
        TransformConfig config_;
    };

    /// \brief Function obfuscation transform. This class automatically discards bb/node/insn callbacks,
    /// just a nice thing to have if you don't want to override 3 unused methods every time.
    /// \tparam Img
    template <pe::any_image_t Img>
    class FunctionTransform : public Transform<Img> {
    public:
        friend Transform<Img>;

        /// \brief Callback that initializes `features_set_`
        void init_features() override {
            this->feature(TransformFeaturesSet::Index::HAS_FUNCTION_TRANSFORM, true);
        }

        /// \brief Transform basic block
        void run_on_bb(TransformContext& /*ctx*/, Function<Img>* /*function*/, analysis::bb_t* /*bb*/) override { }

        /// \brief Transform zasm node
        void run_on_node(TransformContext& /*ctx*/, Function<Img>* /*function*/, zasm::Node* /*node*/) override { }

        /// \brief Transform analysis insn
        void run_on_insn(TransformContext& /*ctx*/, Function<Img>* /*function*/, analysis::insn_t* /*insn*/) override { }
    };

    /// \brief BB obfuscation transform.
    /// Same as `FunctionTransform`, but it discards func/node/insn transforms instead.
    /// \tparam Img
    template <pe::any_image_t Img>
    class BBTransform : public Transform<Img> {
    public:
        friend Transform<Img>;

        /// \brief Callback that initializes `features_set_`
        void init_features() override {
            this->feature(TransformFeaturesSet::Index::HAS_BB_TRANSFORM, true);
        }

        /// \brief Transform routine
        void run_on_function(TransformContext& /*ctx*/, Function<Img>* /*function*/) override { }

        /// \brief Transform zasm node
        void run_on_node(TransformContext& /*ctx*/, Function<Img>* /*function*/, zasm::Node* /*node*/) override { }

        /// \brief Transform analysis insn
        void run_on_insn(TransformContext& /*ctx*/, Function<Img>* /*function*/, analysis::insn_t* /*insn*/) override { }
    };

    /// \brief Node obfuscation transform.
    /// Same as `FunctionTransform`, but it discards bb/func/insn transforms instead.
    /// \tparam Img
    template <pe::any_image_t Img>
    class NodeTransform : public Transform<Img> {
    public:
        friend Transform<Img>;

        /// \brief Callback that initializes `features_set_`
        void init_features() override {
            this->feature(TransformFeaturesSet::Index::HAS_NODE_TRANSFORM, true);
        }

        /// \brief Transform routine
        void run_on_function(TransformContext& /*ctx*/, Function<Img>* /*function*/) override { }

        /// \brief Transform basic block
        void run_on_bb(TransformContext& /*ctx*/, Function<Img>* /*function*/, analysis::bb_t* /*bb*/) override { }

        /// \brief Transform analysis insn
        void run_on_insn(TransformContext& /*ctx*/, Function<Img>* /*function*/, analysis::insn_t* /*insn*/) override { }
    };

    /// \brief Instruction obfuscation transform.
    /// Same as `FunctionTransform`, but it discards bb/func/node transforms instead.
    /// \tparam Img
    template <pe::any_image_t Img>
    class InstructionTransform : public Transform<Img> {
    public:
        friend Transform<Img>;

        /// \brief Callback that initializes `features_set_`
        void init_features() override {
            this->feature(TransformFeaturesSet::Index::HAS_INSN_TRANSFORM, true);
        }

        /// \brief Transform routine
        void run_on_function(TransformContext& /*ctx*/, Function<Img>* /*function*/) override { }

        /// \brief Transform basic block
        void run_on_bb(TransformContext& /*ctx*/, Function<Img>* /*function*/, analysis::bb_t* /*bb*/) override { }

        /// \brief Transform zasm node
        void run_on_node(TransformContext& /*ctx*/, Function<Img>* /*function*/, zasm::Node* /*node*/) override { }
    };
} // namespace obfuscator