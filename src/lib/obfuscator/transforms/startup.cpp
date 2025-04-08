#include "obfuscator/transforms/scheduler.hpp"
#include "obfuscator/transforms/transforms/bogus_control_flow.hpp"
#include "obfuscator/transforms/transforms/constant_crypt.hpp"
#include "obfuscator/transforms/transforms/decomp_break.hpp"
#include "obfuscator/transforms/transforms/substitution.hpp"

namespace obfuscator {
    /// \fixme @es3n1n: This could and should be moved to the transform scheduler constructor
    /// so that when we call singleton ::get() it would init it only once
    void startup_scheduler() {
        auto& scheduler = TransformScheduler::get();

        scheduler.register_transform<transforms::ConstantCrypt>();
        scheduler.register_transform<transforms::Substitution>();
        scheduler.register_transform<transforms::BogusControlFlow>();
        scheduler.register_transform<transforms::DecompBreak>();
    }
} // namespace obfuscator