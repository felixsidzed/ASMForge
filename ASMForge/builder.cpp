#include "builder.h"

#include <iostream>

namespace ASMForge {
	SubRoutineBuilder::SubRoutineBuilder(const std::string& routineName, ModuleBuilder* parentModule) 
		: name(routineName), M(parentModule) {}

	void SubRoutineBuilder::Append(const Instruction& instr) {
		code.push_back(instr);
	}

	const std::vector<Instruction>& SubRoutineBuilder::GetCode() const {
		return code;
	}

	// error: _is64Bit undeclared identifer ???
	ModuleBuilder::ModuleBuilder(const std::string& moduleName, bool _is64bit) : name(moduleName) {
		this->is64Bit = _is64bit;
	}

	SubRoutineBuilder* ModuleBuilder::CreateSubRoutine(const std::string& routineName) {
		for (const auto& subRoutine : GetSubRoutines()) {
			if (subRoutine->name == routineName) {
				std::cerr << "A subroutine with the name '" << routineName << "' already exists." << std::endl;
				std::terminate();
			}
		}

		auto R = std::make_unique<SubRoutineBuilder>(routineName, this);
		SubRoutineBuilder* ptr = R.get();
		subRoutines.push_back(std::move(R));
		return ptr;
	}

	const std::vector<std::unique_ptr<SubRoutineBuilder>>& ModuleBuilder::GetSubRoutines() const {
		return subRoutines;
	}
}
