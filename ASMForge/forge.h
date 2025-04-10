#pragma once

#include "builder.h"

namespace ASMForge {
	ModuleBuilder* New(const std::string& name, bool is64Bit = true);
}
