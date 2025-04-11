#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <memory>
#include <iostream>

#ifndef NDEBUG
#define assert(condition, message) \
	if (!(condition)) { \
		std::cerr << "Assertion `" #condition "` failed in " << __FILE__ \
					<< " line " << __LINE__ << ": " << message << std::endl; \
		std::terminate(); \
	}
#else
#define assert(condition, message) (void)0
#endif

namespace ASMForge {
	enum class OpCode : uint8_t {
		ADD,
		SUB,
		MOV,
		POP,
		PUSH,
		HLT = 0xF4,
		RET = 0xC3,
		NOP = 0x90
	};

	enum class Reg : uint8_t {
		// No R/E prefix because the emitter checks if the module is 64 bit for R prefix and E prefix if not
		AX = 0x0,
		CX = 0x1,
		DX = 0x2,
		BX = 0x3,
		SP = 0x4,
		BP = 0x5,
		SI = 0x6,
		DI = 0x7
	};

	enum class InstrType : uint8_t {
		// R = register
		// M = memory
		// I = immediate
		R,
		RR,
		RM,
		RI,
		MR,
		NONE
	};

	enum class GlobalType : uint8_t {
		None = 0x0,
		Function = 0x20,
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

	class Global;
	class SubRoutineBuilder;

	class ModuleBuilder {
	public:
		std::string name;
		std::vector<std::unique_ptr<Global>> globals;
		std::vector<std::unique_ptr<SubRoutineBuilder>> subRoutines;

		bool is64Bit;

		explicit ModuleBuilder(const std::string& moduleName, bool _is64Bit);

		SubRoutineBuilder* CreateSubRoutine(const std::string& name);
		const std::vector<std::unique_ptr<SubRoutineBuilder>>& GetSubRoutines() const;

		uint8_t* EmitToMemory(size_t* size);
		uint8_t* EmitCOFF(size_t* size);

		Global* MakeGlobal(SubRoutineBuilder* func);
		const std::vector<std::unique_ptr<Global>>& GetGlobals() const;
	};

	class ExtendedModuleBuilder : public ModuleBuilder {
	public:	
		explicit ExtendedModuleBuilder(const std::string& moduleName, bool _is64Bit)
			: ModuleBuilder(moduleName, _is64Bit) {
		}

		SubRoutineBuilder* CreateSubRoutine(const std::string& name);
	};

	class Global {
	public:
		ModuleBuilder* M;
		std::string name;
		uintptr_t value;
		GlobalType type;

		Global(const std::string& name, ModuleBuilder* M);
	};

	class SubRoutineBuilder {
	public:
		ModuleBuilder* M;
		std::string name;
		std::vector<Instruction> code;

		SubRoutineBuilder(const std::string& routineName, ModuleBuilder* parentModule);

		void Append(const Instruction& instruction);

		void SetFunctionBodyPosition(uint64_t pos = 0) { assert(pos > 0 && pos < code.size(), "Out of range"); functionBodyPos = pos; };
		uint64_t GetFunctionBodyPosition() { return functionBodyPos; };

		Instruction Create(OpCode op, uint64_t a = 0, uint64_t b = 0, InstrType type = InstrType::NONE) {
			return Instruction(op, a, b, M->is64Bit, type);
		};
		Instruction Create(OpCode op, Reg a, uint64_t b) {
			return Create(op, static_cast<uint64_t>(a), b, InstrType::RI);
		};
		Instruction Create(OpCode op, Reg a, Reg b) {
			return Create(op, static_cast<uint64_t>(a), static_cast<uint64_t>(b), InstrType::RR);
		};
		Instruction Create(OpCode op, Reg a) {
			return Create(op, static_cast<uint64_t>(a), 0, InstrType::R);
		}

		const std::vector<Instruction>& GetCode() const;

		uint8_t* EmitToMemory(size_t* size);
	private:
		uint64_t functionBodyPos;
	};
}
