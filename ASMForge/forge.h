#pragma once

#include "builder.h"

namespace ASMForge {

#if defined(_WIN64) || defined(__x86_64__) || defined(__ppc64__)
    ExtendedModuleBuilder* New(const std::string& name = "module", bool is64Bit = true);
#elif defined(_WIN32) || defined(__GNUC__)
    ExtendedModuleBuilder* New(const std::string& name = "module", bool is64Bit = false);
#endif
}
