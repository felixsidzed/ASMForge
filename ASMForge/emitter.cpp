#include "builder.h"

#include <iostream>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

struct COFFHeader {
	uint16_t machine;
	uint16_t numSections;
	uint32_t timestamp;
	uint32_t symbolTablePointer;
	uint32_t numSymbols;
	uint16_t optionalHeaderSize;
	uint16_t characteristics;
};

struct COFFSymbol {
	union {
		char name[8];
		uint32_t namePointer;
	};
	uint32_t value;
	uint16_t sectionNumber;
	uint16_t type;
	uint8_t storageClass;
	uint8_t numAuxEntries;
};

struct SectionHeader {
	char name[8];
	uint32_t virtualSize;
	uint32_t virtualAddress;
	uint32_t sizeOfRawData;
	uint32_t pointerToRawData;
	uint32_t pointerToRelocations;
	uint32_t pointerToLinenumbers;
	uint16_t numRelocations;
	uint16_t numLinenumbers;
	uint32_t characteristics;
};

#ifndef NDEBUG
	#define assert(condition, message) \
	do { \
		if (! (condition)) { \
			std::cerr << "Assertion `" #condition "` failed in " << __FILE__ \
						<< " line " << __LINE__ << ": " << message << std::endl; \
			std::terminate(); \
		} \
	} while (false)
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

	uint8_t* ModuleBuilder::EmitCOFF(size_t* size) {
		size_t sizeCode = 0;
		uint8_t* code = EmitToMemory(&sizeCode);
		if (!code || sizeCode == 0) {
			*size = 0;
			return nullptr;
		}

		COFFHeader header = {};
		header.machine = is64Bit ? 0x8664 : 0x14c;
		header.timestamp = static_cast<uint32_t>(time(nullptr));
		header.numSections = 1;
		header.numSymbols = 1;
		header.symbolTablePointer = sizeof(COFFHeader) + sizeof(IMAGE_OPTIONAL_HEADER) + sizeof(SectionHeader) + sizeCode;
		header.optionalHeaderSize = sizeof(IMAGE_OPTIONAL_HEADER);
		header.characteristics = 0x0002 | 0x0100; // EXECUTABLE_IMAGE | 32BIT_MACHINE

		// TODO: This is more common in PE files, not COFF files
		if (is64Bit)
			header.characteristics |= 0x0020; // LARGE_ADDRESS_AWARE

		SectionHeader text = {};
		strcpy_s(text.name, ".text"); // assert failed here: Buffer is too small.
		text.virtualSize = (uint32_t)sizeCode;
		text.virtualAddress = 0x1000;
		text.sizeOfRawData = (uint32_t)sizeCode;
		text.pointerToRawData = sizeof(COFFHeader) + sizeof(IMAGE_OPTIONAL_HEADER) + sizeof(SectionHeader);
		text.pointerToRelocations = 0;
		text.pointerToLinenumbers = 0;
		text.numRelocations = 0;
		text.numLinenumbers = 0;
		text.characteristics =
			0x00000020 | // IMAGE_SCN_CNT_CODE
			0x20000000 | // IMAGE_SCN_MEM_EXECUTE
			0x40000000;  // IMAGE_SCN_MEM_READ

		size_t totalSize = sizeof(COFFHeader) + sizeof(IMAGE_OPTIONAL_HEADER) + sizeof(SectionHeader) + sizeCode + (sizeof(COFFSymbol) * header.numSymbols);
		uint8_t* buf = new uint8_t[totalSize];

		size_t offset = 0;

		// No idea how to fix this warning 🤷‍
		// I'm pretty sure it's just MSVC acting out and not an issue on my end
		#pragma warning(push)
		#pragma warning(disable : 6386)
		memcpy(buf, &header, sizeof(COFFHeader));
		offset += sizeof(COFFHeader);
		#pragma warning(pop)

		IMAGE_OPTIONAL_HEADER optionalHeader = {};
		optionalHeader.Magic = is64Bit ? 0x020b : 0x010b;
		optionalHeader.AddressOfEntryPoint = text.virtualAddress;
		optionalHeader.ImageBase = 0x40000;

		memcpy(buf + offset, &optionalHeader, sizeof(IMAGE_OPTIONAL_HEADER));
		offset += sizeof(IMAGE_OPTIONAL_HEADER);

		memcpy(buf + offset, &text, sizeof(SectionHeader));
		offset += sizeof(SectionHeader);

		memcpy(buf + offset, code, sizeCode);
		offset += sizeCode;

		// TODO: This doesnt seem to work...
		COFFSymbol symEntry = {};
		strcpy_s(symEntry.name, 8, "main");
		symEntry.value = text.virtualAddress;
		symEntry.sectionNumber = 1;
		symEntry.type = 0x20;
		symEntry.storageClass = IMAGE_SYM_CLASS_EXTERNAL;
		symEntry.numAuxEntries = 0;

		memcpy(buf + offset, &symEntry, sizeof(COFFSymbol));
		offset += sizeof(COFFSymbol);

		delete[] code;
		*size = totalSize;

		return buf;
	}
}
