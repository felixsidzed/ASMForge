#include "builder.h"

#include <iostream>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#pragma pack(push, 1)
struct CoffFileHeader {
	uint16_t Machine;
	uint16_t NumberOfSections;
	uint32_t TimeDateStamp = std::time(nullptr);
	uint32_t PointerToSymbolTable;
	uint32_t NumberOfSymbols;
	uint16_t SizeOfOptionalHeader;
	uint16_t Characteristics;
};

struct SectionHeader {
	char Name[8];
	uint32_t VirtualSize;
	uint32_t VirtualAddress;
	uint32_t SizeOfRawData;
	uint32_t PointerToRawData;
	uint32_t PointerToRelocations;
	uint32_t PointerToLinenumbers;
	uint16_t NumberOfRelocations;
	uint16_t NumberOfLinenumbers;
	uint32_t Characteristics;
};

struct CoffSymbol {
	union {
		char ShortName[8];
		struct {
			uint32_t Zeroes;
			uint32_t Offset;
		};
	};
	uint32_t Value;
	int16_t SectionNumber;
	uint16_t Type;
	uint8_t StorageClass;
	uint8_t NumberOfAuxSymbols;
};
#pragma pack(pop, 1)

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
						Emit(buf, static_cast<uint8_t>(0x88 | (instr.A << 3) | instr.B));
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
					case InstrType::RR: {
						Emit(buf, 0x01);
						Emit(buf, static_cast<uint8_t>((instr.A << 3) | instr.B));
						break;
					}
					case InstrType::RM: {
						Emit(buf, 0x03);
						Emit(buf, static_cast<uint8_t>((instr.A << 3) | instr.B));
						break;
					}
					case InstrType::RI: {
						Emit(buf, 0x81);
						Emit(buf, static_cast<uint8_t>((instr.A << 3) | instr.B));
						Emit(buf, instr.B, instr.is64);
						break;
					}
					default:
						assert(false, "Unsupported instruction type");
						break;
					}
					break;
				}

				case OpCode::SUB: {
					switch (instr.type) {
					case InstrType::RR: {
						Emit(buf, 0x29);
						Emit(buf, static_cast<uint8_t>((instr.A << 3) | instr.B));
						break;
					}
					case InstrType::RM: {
						Emit(buf, 0x2B);
						Emit(buf, static_cast<uint8_t>((instr.A << 3) | instr.B));
						break;
					}
					case InstrType::RI: {
						Emit(buf, 0x81);
						Emit(buf, static_cast<uint8_t>((instr.A << 3) | instr.B));
						Emit(buf, instr.B, instr.is64);
						break;
					}
					default:
						assert(false, "Unsupported instruction type");
						break;
					}
					break;
				}

				default:
					if (instr.is64) buf.pop_back(); // Instructions without operands don't have a REX.W prefix, I think?
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

	uint8_t* ModuleBuilder::EmitCOFF(size_t* size, const std::string& entry) {
		bool hasEntryPoint = false;
		for (const auto& R : GetSubRoutines()) {
			if (R->name == entry) {
				hasEntryPoint = true;
			}
		}
		assert(hasEntryPoint, "No entry point found.")

		size_t sizeCode = 0;
		uint8_t* code = EmitToMemory(&sizeCode);
		if (!code || sizeCode == 0) {
			*size = 0;
			return nullptr;
		}

		CoffFileHeader coffHeader = {};
		SectionHeader textHeader = {};

		uint32_t afterHeaders = sizeof(CoffFileHeader) + sizeof(SectionHeader);

		textHeader.SizeOfRawData = (uint32_t)sizeCode;
		textHeader.PointerToRawData = afterHeaders;
		textHeader.VirtualSize = (uint32_t)sizeCode;
		memcpy(textHeader.Name, ".text\0\0\0", 5);
		textHeader.Characteristics = 0x60000020; // code | execute | read

		coffHeader.Machine = 0x8664;
		coffHeader.NumberOfSections = 1;
		coffHeader.TimeDateStamp = (uint32_t)time(nullptr);
		coffHeader.PointerToSymbolTable = afterHeaders + (uint32_t)sizeCode;
		coffHeader.NumberOfSymbols = 1;
		coffHeader.SizeOfOptionalHeader = 0;

		if (is64Bit)
			coffHeader.Characteristics |= 0x0020; // IMAGE_FILE_LARGE_ADDRESS_AWARE
		else
			coffHeader.Machine = 0x14c; // IMAGE_FILE_MACHINE_I386

		CoffSymbol entrySymbol = {};
		strncpy_s(entrySymbol.ShortName, entry.c_str(), 8);
		entrySymbol.Value = 0;
		entrySymbol.SectionNumber = 1;
		entrySymbol.Type = 0x20;
		entrySymbol.StorageClass = 2;
		entrySymbol.NumberOfAuxSymbols = 0;

		uint32_t stringTableSize = 4;

		*size = sizeof(CoffFileHeader) +
			sizeof(SectionHeader) +
			(uint32_t)sizeCode +
			sizeof(CoffSymbol) +
			sizeof(uint32_t);

		uint8_t* buffer = new uint8_t[*size];
		uint8_t* ptr = buffer;

		memcpy(ptr, &coffHeader, sizeof(CoffFileHeader));
		ptr += sizeof(CoffFileHeader);

		memcpy(ptr, &textHeader, sizeof(SectionHeader));
		ptr += sizeof(SectionHeader);

		memcpy(ptr, code, (uint32_t)sizeCode);
		ptr += (uint32_t)sizeCode;

		memcpy(ptr, &entrySymbol, sizeof(CoffSymbol));
		ptr += sizeof(CoffSymbol);

		memcpy(ptr, &stringTableSize, sizeof(uint32_t));
		ptr += stringTableSize;

		return buffer;
	}
}
