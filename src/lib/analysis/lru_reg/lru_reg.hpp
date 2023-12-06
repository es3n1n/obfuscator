#pragma once
#include "easm/easm.hpp"
#include "pe/pe.hpp"
#include "util/random.hpp"

#include <list>
#include <unordered_set>

namespace analysis {
    using RegID = zasm::Reg::Id;

    namespace detail {
        inline std::array kProtectedRegisters = {RegID{ZYDIS_REGISTER_RSP}, RegID{ZYDIS_REGISTER_RBP}, RegID{ZYDIS_REGISTER_RIP}};
        inline std::array kRegistersX86 = {
            RegID{ZYDIS_REGISTER_RAX}, RegID{ZYDIS_REGISTER_RBX}, RegID{ZYDIS_REGISTER_RCX}, RegID{ZYDIS_REGISTER_RDX},
            RegID{ZYDIS_REGISTER_RSI}, RegID{ZYDIS_REGISTER_RDI}, RegID{ZYDIS_REGISTER_RBP}, RegID{ZYDIS_REGISTER_RSP},
        };
        inline std::array kRegistersX64 = {
            RegID{ZYDIS_REGISTER_R8},  RegID{ZYDIS_REGISTER_R9},  RegID{ZYDIS_REGISTER_R10}, RegID{ZYDIS_REGISTER_R11},
            RegID{ZYDIS_REGISTER_R12}, RegID{ZYDIS_REGISTER_R13}, RegID{ZYDIS_REGISTER_R14}, RegID{ZYDIS_REGISTER_R15},
        };
    } // namespace detail

    /// \brief Least recently used register container
    class LRURegContainer {
    public:
        DEFAULT_CTOR_DTOR(LRURegContainer);
        DEFAULT_COPY(LRURegContainer);

        /// \brief Emplace register to the cache
        /// \param reg_id Register id
        void push(const RegID reg_id) {
            /// Mark as recently used
            if (cache_.contains(reg_id)) {
                items_.remove(reg_id);
                items_.push_front(reg_id);
                return;
            }

            /// Don't store protected registers
            if (std::ranges::find(detail::kProtectedRegisters, zasm::Reg{reg_id}.getRoot(zasm::MachineMode::AMD64).getId()) !=
                std::end(detail::kProtectedRegisters)) {
                return;
            }

            /// Otherwise insert
            items_.push_front(reg_id);
            cache_.insert(reg_id);
        }

        /// \brief Get the least recently used register
        /// \param return_random should we choose a random register across least recently used registers?
        /// \return Register id
        [[nodiscard]] RegID get(const bool return_random = false) {
            /// Return random, if needed
            if (return_random) {
                return random();
            }

            return filter([this]() -> RegID {
                /// Get the last recently used item
                const auto lru = items_.back();

                /// Mark as recently used
                items_.remove(lru);
                items_.push_front(lru);
                return lru;
            });
        }

        /// \brief Get a random register across least used registers cache
        /// \return Register id
        [[nodiscard]] RegID random() {
            return filter([this]() -> RegID {
                /// Get a random register
                const auto lru = rnd::item(items_);

                /// Mark as recently used
                items_.remove(lru);
                items_.push_front(lru);
                return lru;
            });
        }

        /// \brief Temporary blacklist register
        /// \param reg_id register
        void blacklist(const RegID reg_id) {
            blacklisted_.insert(reg_id);
        }

        /// \brief Clear blacklist
        void clear_blacklist() {
            blacklisted_.clear();
        }

        /// \brief An object that will clear blacklist in its destructor
        struct blacklist_state_t {
            explicit blacklist_state_t(LRURegContainer* container): container(container) { }
            ~blacklist_state_t() {
                container->clear_blacklist();
            }

            LRURegContainer* container = nullptr;
        };

        /// \brief Create a raii auto blacklist cleaner
        /// \return RAII object that will clear the blacklist in dctor
        [[nodiscard]] blacklist_state_t auto_cleaner() noexcept {
            return blacklist_state_t(this);
        }

    private:
        /// \brief Filter out blacklisted values
        /// \param callback callback that should return RegID
        /// \return filtered RegID that isn't blacklisted
        [[nodiscard]] RegID filter(const std::function<RegID()>& callback) const {
            RegID result;

            do {
                result = callback();
            } while (blacklisted_.contains(result));

            return result;
        }

        /// \brief Temporary blacklisted registers
        std::unordered_set<RegID> blacklisted_ = {};
        /// \brief Items storage itself
        std::list<RegID> items_ = {};
        /// \brief Unordered set for a bit faster contains checks
        std::unordered_set<RegID> cache_ = {};
    };

    /// \brief An lru cache for all types of GP registers
    /// \tparam Img X64 or X86 image, depending on this image, the gp64 lru cache could be available
    template <pe::any_image_t Img>
    class LRUReg {
        constexpr static bool IsX64 = pe::is_x64_v<Img>;
        using RegTy = zasm::x86::Gp;

    public:
        DEFAULT_DTOR(LRUReg);
        DEFAULT_COPY(LRUReg);

        LRUReg() {
            /// Push X86 registers
            for (const auto reg_id : detail::kRegistersX86) {
                push(reg_id);
            }

            /// Push x64 registers if needed
            if constexpr (IsX64) {
                for (const auto reg_id : detail::kRegistersX64) {
                    push(reg_id);
                }
            }
        }

        /// \brief Push register to the lru cache
        /// \param reg_id register
        void push(const RegID reg_id) {
            storage_.push(to_gp_ptr(reg_id));
        }

        /// \brief Temporary blacklist register
        /// \param reg_id register
        void blacklist(const RegID reg_id) {
            storage_.blacklist(to_gp_ptr(reg_id));
        }

        /// \brief Clear blacklist
        void clear_blacklist() {
            storage_.clear_blacklist();
        }

        /// \brief Create a raii auto blacklist cleaner
        /// \return RAII object that will clear the blacklist in dctor
        [[nodiscard]] LRURegContainer::blacklist_state_t auto_cleaner() noexcept {
            return storage_.auto_cleaner();
        }

        /// \brief Get least recently used register as Gp8
        /// \param random should we choose a random register across least recently used registers?
        /// \return Register
        [[nodiscard]] RegTy get_gp8_lo(const bool random = false) {
            return RegTy{easm::reg_convert::gp64_to_gp8(to_gp64_if_needed(storage_.get(random)))};
        }

        /// \brief Get least recently used register as Gp16
        /// \param random should we choose a random register across least recently used registers?
        /// \return Register
        [[nodiscard]] RegTy get_gp16_lo(const bool random = false) {
            return RegTy{easm::reg_convert::gp64_to_gp16(to_gp64_if_needed(storage_.get(random)))};
        }

        /// \brief Get least recently used register as Gp32
        /// \param random should we choose a random register across least recently used registers?
        /// \return Register
        [[nodiscard]] RegTy get_gp32_lo(const bool random = false) {
            return RegTy{easm::reg_convert::gp64_to_gp32(to_gp64_if_needed(storage_.get(random)))};
        }

        /// \brief Get least recently used register as Gp64
        /// \param random should we choose a random register across least recently used registers?
        /// \return Register
        [[nodiscard]] RegTy get_gp64(const bool random = false) {
            assert(IsX64); // no gp64 registers on x86
            return RegTy{to_gp64_if_needed(storage_.get(random))};
        }

        /// \brief Get least recently used register (gp64 for x64 and gp32 for x86)
        /// \param random should we choose a random register across least recently used registers?
        /// \return Register
        [[nodiscard]] RegTy get(const bool random = false) {
            return RegTy{storage_.get(random)};
        }

        /// \brief Get register with size passed as `bit_size`
        /// \param bit_size register bit size
        /// \param random should we choose a random register across least recently used registers?
        /// \return Register
        [[nodiscard]] RegTy get_for_bits(const zasm::BitSize bit_size, const bool random = false) {
            switch (getBitSize(bit_size)) {
            case 8:
                return get_gp8_lo(random);
            case 16:
                return get_gp16_lo(random);
            case 32:
                return get_gp32_lo(random);
            case 64:
                return get_gp64(random);
            default:
                break;
            }
            throw std::runtime_error("lru_reg: unsupported bit size");
        }

        /// \brief Get machine mode based on the image tparam type
        /// \return machine_mode
        [[nodiscard]] static zasm::MachineMode machine_mode() noexcept {
            return IsX64 ? zasm::MachineMode::AMD64 : zasm::MachineMode::I386;
        }

        /// \brief Converts any gp register to its base register, gp64 for x64 and gp32 for x86
        /// \param reg_id register that it should convert
        /// \return Converted register id
        [[nodiscard]] static RegID to_gp_ptr(const RegID reg_id) noexcept {
            const auto reg = zasm::Reg{reg_id};
            return reg.getRoot(machine_mode()).getId();
        }

        /// \brief Convert to GP64, if needed
        /// \param reg_id register that it should convert
        /// \return Converted GP64 reg id
        [[nodiscard]] static RegID to_gp64_if_needed(const RegID reg_id) noexcept {
            const auto gp_ptr = to_gp_ptr(reg_id);
            return IsX64 ? gp_ptr : easm::reg_convert::gp32_to_gp64(gp_ptr);
        }

    private:
        /// \brief gp uptr lru container
        LRURegContainer storage_ = {};
    };
} // namespace analysis