#include "forge.h"

#include <iostream>

namespace ASMForge {
	ExtendedModuleBuilder* New(const std::string& name, bool is64Bit) {
		ExtendedModuleBuilder* M = new ExtendedModuleBuilder(name, is64Bit);

		return M;
	}
}
