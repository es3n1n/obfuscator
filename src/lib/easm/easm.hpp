#pragma once

/// \note @es3n1n: Just some general notes
/// - I'm not really a fan of passing machine mode everywhere we can since there could be only 1 machine mode
///     per binary, so we can/should probbaly store it somewhere globally me thinks

#include "easm/assembler/assembler.hpp"
#include "easm/disassembler/disassembler.hpp"
#include "easm/misc/misc.hpp"
#include "easm/misc/reg_convert.hpp"
