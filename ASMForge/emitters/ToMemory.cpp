#include "../builder.h"

#include <iostream>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace ASMForge {
	static void Emit(std::vector<uint8_t>& buf, uint8_t byte) {
		buf.push_back(byte);
	}

	template<uint16_t byteCount>
	static void Emit(std::vector<uint8_t>& buf, uint64_t imm) {
		for (int i = 0; i < byteCount; ++i) {
			buf.push_back((imm >> (i * 8)) & 0xFF);
		}
	}

	static void Emit(std::vector<uint8_t>& buf, uint64_t imm, bool _64) {
		if (_64) Emit<8>(buf, imm);
		else Emit<4>(buf, imm);
	}

	uint8_t* SubRoutineBuilder::EmitToMemory(size_t* size) {
		std::vector<uint8_t> buf;

		for (const auto& instr : code) {
			if (instr.is64) {
				Emit(buf, 0x48);
			}

			switch (instr.op) {
				case OpCode::MOV: {
					switch (instr.type) {
					case InstrType::RR:
						Emit(buf, 0x89);
						Emit(buf, static_cast<uint8_t>((0x3 << 6) | (instr.B << 3) | instr.A));
						break;
					case InstrType::RM:
						Emit(buf, 0x8B);
						Emit(buf, static_cast<uint8_t>((instr.A << 3) | instr.B));
						break;
					case InstrType::RI:
						Emit(buf, 0xB8 | (instr.A & 0x7));
						Emit(buf, instr.B, instr.is64);
						break;
					default:
						assert(false, "Unsupported instruction type");
						break;
					}
					break;
				}

				case OpCode::ADD: {
					switch (instr.type) {
					case InstrType::RI:
						Emit(buf, 0x81);
						Emit(buf, static_cast<uint8_t>(0xC0 | (instr.A << 3)));
						Emit(buf, instr.B, instr.is64);
						break;
					case InstrType::RR:
						Emit(buf, 0x01);
						Emit(buf, static_cast<uint8_t>(0xC0 | (instr.B << 3) | instr.A));
						break;
					default:
						assert(false, "Unsupported instruction type");
						break;
					}
					break;
				}

				case OpCode::PUSH: {
					assert(instr.type == InstrType::R, "Unsupported instruction type");
					Emit(buf, 0x50 + static_cast<uint8_t>(instr.A));
					break;
				}

				case OpCode::POP: {
					assert(instr.type == InstrType::R, "Unsupported instruction type");
					Emit(buf, 0x58 + static_cast<uint8_t>(instr.A));
					break;
				}

				default:
					// Instructions without operands don't have a REX.W prefix, I think?
					if (instr.is64) buf.pop_back();
					Emit(buf, static_cast<uint8_t>(instr.op));
					break;
				}
		}

		if (size)
			*size = buf.size();

		uint8_t* out = new uint8_t[buf.size()];
		memcpy(out, buf.data(), buf.size());

		return out;
	}

	uint8_t* ModuleBuilder::EmitToMemory(size_t* size) {
		std::vector<uint8_t> buf;

		for (const auto& R : subRoutines) {
			size_t sizeCode = 0;
			uint8_t* code = R->EmitToMemory(&sizeCode);

			buf.insert(buf.end(), code, code + sizeCode);

			delete[] code;
		}

		*size = buf.size();
		uint8_t* moduleMemory = new uint8_t[buf.size()];
		std::copy(buf.begin(), buf.end(), moduleMemory);

		return moduleMemory;
	}
}
