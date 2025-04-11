#include "builder.h"

#include <iostream>

namespace ASMForge {
	SubRoutineBuilder::SubRoutineBuilder(const std::string& routineName, ModuleBuilder* parentModule) 
		: name(routineName), M(parentModule), functionBodyPos(0) {}

	void SubRoutineBuilder::Append(const Instruction& instr) {
		code.insert(code.begin() + functionBodyPos, instr);
		functionBodyPos++;
	}

	const std::vector<Instruction>& SubRoutineBuilder::GetCode() const {
		return code;
	}

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

	Global* ModuleBuilder::MakeGlobal(SubRoutineBuilder* func) {
		auto global = std::make_unique<Global>(func->name, this);

		global->value = (uintptr_t)func;
		global->type = GlobalType::Function;

		Global* ptr = global.get();
		globals.push_back(std::move(global));
		return ptr;
	}

	const std::vector<std::unique_ptr<Global>>& ModuleBuilder::GetGlobals() const {
		return globals;
	}

	SubRoutineBuilder* ExtendedModuleBuilder::CreateSubRoutine(const std::string& name) {
		SubRoutineBuilder* R = ModuleBuilder::CreateSubRoutine(name);

		R->Append(R->Create(OpCode::PUSH, Reg::BP));
		R->Append(R->Create(OpCode::MOV, Reg::BP, Reg::SP));

		uint64_t a = R->GetFunctionBodyPosition();

		R->Append(R->Create(OpCode::MOV, Reg::SP, Reg::BP));
		R->Append(R->Create(OpCode::POP, Reg::BP));
		R->Append(R->Create(OpCode::RET));

		R->SetFunctionBodyPosition(a);

		return R;
	}

	Global::Global(const std::string& name, ModuleBuilder* M)
		: name(name), M(M), value(0), type(GlobalType::None) {};
}
