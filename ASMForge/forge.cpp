#include "forge.h"

#include <iostream>

namespace ASMForge {
	ModuleBuilder* New(const std::string& name, bool is64Bit) {
		ModuleBuilder* M = new ModuleBuilder(name, is64Bit);

		return M;
	}
}
