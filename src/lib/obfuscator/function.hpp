#pragma once
#include "analysis/var_alloc/var_alloc.hpp"
#include "obfuscator/transforms/types.hpp"

namespace obfuscator {
    /// \brief Function information representation
    /// \tparam Img PE Image type, either x64 or x86
    template <pe::any_image_t Img>
    struct Function {
        DEFAULT_CTOR_DTOR(Function);
        NON_COPYABLE(Function);
        Function(const analysis::Function<Img>& func, const Img* image)
            : parsed_func(func.parsed_func), lru_reg(func.lru_reg), bb_storage(func.bb_storage), program(func.program), assembler(func.assembler),
              cursor(std::make_unique<easm::Cursor>(program, assembler)), observer(func.observer), bb_provider(func.bb_provider),
              machine_mode(image->guess_machine_mode()) { }

        /// \brief Construct new var allocator
        /// \return varalloc instance
        auto var_alloc() {
            return analysis::VarAlloc<Img>(&lru_reg);
        }

        /// \brief Parsed information from PDB/MAP/etc
        func_parser::function_t parsed_func;
        /// \brief Least recently used register cache
        analysis::LRUReg<Img> lru_reg;
        /// \brief A storage with basic blocks
        std::shared_ptr<analysis::bb_storage_t> bb_storage;
        /// \brief Zasm routine
        std::shared_ptr<zasm::Program> program;
        /// \brief Zasm assembler
        std::shared_ptr<zasm::x86::Assembler> assembler;
        /// \brief Assembler cursor wrapped
        std::unique_ptr<easm::Cursor> cursor;
        /// \brief Node observer
        std::shared_ptr<analysis::Observer> observer;
        /// \brief BB Provider
        std::shared_ptr<analysis::functional_bb_provider_t> bb_provider;
        /// \brief Zasm machine mode
        const zasm::MachineMode machine_mode;
    };
} // namespace obfuscator