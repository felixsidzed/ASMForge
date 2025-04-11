#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <memory>

namespace ASMForge {
	enum class OpCode : uint8_t {
		MOV = 0x01,
		ADD = 0x02,
		SUB = 0x03,
		HLT = 0xF4,
		RET = 0xC3
	};

	enum class Reg : uint8_t {
		AX = 0x0,
		CX = 0x1,
		DX = 0x2
	};

	enum class InstrType : uint8_t {
		// R = register
		// M = memory
		// I = immediate
		RR,
		RM,
		RI,
		MR,
		NONE
	};

	struct Instruction {
		OpCode op;
		uint64_t A;
		uint64_t B;
		InstrType type;

		bool is64;

		Instruction(OpCode opCode, uint64_t A = 0, uint64_t B = 0, bool is64 = true, InstrType type = InstrType::NONE)
			: op(opCode), A(A), B(B), is64(is64), type(type) {}
	};

	class SubRoutineBuilder;

	class ModuleBuilder {
	public:
		std::string name;
		std::vector<std::unique_ptr<SubRoutineBuilder>> subRoutines;

		bool is64Bit;

		explicit ModuleBuilder(const std::string& moduleName, bool _is64Bit);

		SubRoutineBuilder* CreateSubRoutine(const std::string& name);
		const std::vector<std::unique_ptr<SubRoutineBuilder>>& GetSubRoutines() const;

		uint8_t* EmitToMemory(size_t* size);
		uint8_t* EmitCOFF(size_t* size, const std::string& entry = "_main");
	};

	class SubRoutineBuilder {
	public:
		ModuleBuilder* M;
		std::string name;
		std::vector<Instruction> code;

		SubRoutineBuilder(const std::string& routineName, ModuleBuilder* parentModule);

		void Append(const Instruction& instruction);

		Instruction Create(OpCode op, uint64_t a = 0, uint64_t b = 0, InstrType type = InstrType::NONE) {
			return Instruction(op, a, b, (bool)M->is64Bit, type); // here
		};
		Instruction Create(OpCode op, Reg a, uint64_t b = 0) {
			return Create(op, static_cast<uint64_t>(a), b, InstrType::RI);
		};
		Instruction Create(OpCode op, Reg a, Reg b) {
			return Create(op, static_cast<uint64_t>(a), static_cast<uint64_t>(b), InstrType::RR);
		};

		const std::vector<Instruction>& GetCode() const;

		uint8_t* EmitToMemory(size_t* size);
	};
}
