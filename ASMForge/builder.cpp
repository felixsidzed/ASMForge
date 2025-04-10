#include "builder.h"

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
		auto routine = std::make_unique<SubRoutineBuilder>(routineName, this);
		SubRoutineBuilder* ptr = routine.get();
		subRoutines.push_back(std::move(routine));
		return ptr;
	}

	const std::vector<std::unique_ptr<SubRoutineBuilder>>& ModuleBuilder::GetSubRoutines() const {
		return subRoutines;
	}
}
