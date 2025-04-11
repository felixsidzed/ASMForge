#include "../builder.h"

#include <unordered_map>

#pragma pack(push, 1)
struct COFFFileHeader {
	uint16_t Machine;
	uint16_t NumberOfSections;
	uint32_t TimeDateStamp = (uint32_t)time(nullptr);
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

struct COFFSymbol {
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

namespace ASMForge {
	uint8_t* ModuleBuilder::EmitCOFF(size_t* size) {
		uint8_t* code;
		size_t sizeCode = 0;
		std::unordered_map<std::string, uint32_t> functionRvas;

		{
			std::vector<uint8_t> buf;

			for (const auto& R : subRoutines) {
				size_t RSizeCode = 0;
				uint8_t* RCode = R->EmitToMemory(&RSizeCode);

				functionRvas[R->name] = (uint32_t)buf.size();
				buf.insert(buf.end(), RCode, RCode + RSizeCode);

				delete[] RCode;
			}

			sizeCode = buf.size();
			code = new uint8_t[buf.size()];
			std::copy(buf.begin(), buf.end(), code);
		}

		COFFFileHeader coffHeader = {};
		SectionHeader textHeader = {};

		uint32_t afterHeaders = sizeof(COFFFileHeader) + sizeof(SectionHeader);

		textHeader.SizeOfRawData = (uint32_t)sizeCode;
		textHeader.PointerToRawData = afterHeaders;
		textHeader.VirtualSize = (uint32_t)sizeCode;
		memcpy(textHeader.Name, ".text", 5);
		textHeader.Characteristics = 0x60000020; // code | execute | read

		coffHeader.Machine = is64Bit ? 0x8664 : 0x14c;
		coffHeader.NumberOfSections = 1;
		coffHeader.TimeDateStamp = (uint32_t)time(nullptr);
		coffHeader.PointerToSymbolTable = afterHeaders + (uint32_t)sizeCode;
		coffHeader.NumberOfSymbols = 1;
		coffHeader.SizeOfOptionalHeader = 0;

		// TODO: This is more common in PE files, not COFF
		if (is64Bit)
			coffHeader.Characteristics |= 0x0020; // IMAGE_FILE_LARGE_ADDRESS_AWARE

		size_t symCount = 0;
		std::string stringTable;
		uint32_t stringTableSize = 4;
		std::vector<COFFSymbol> symbols;

		for (auto& global : globals) {
			COFFSymbol sym = {};

			std::string name = global->name;

			if (name.length() > 8) {
				sym.Zeroes = 0;
				sym.Offset = stringTableSize;

				stringTable += name + '\0';
				stringTableSize += (uint32_t)name.length() + 1;
			}
			else {
				memset(sym.ShortName, 0, 8);
				memcpy(sym.ShortName, name.c_str(), name.length());
			}

			sym.Value = *(uint32_t*)global->value;
			sym.SectionNumber = 1;
			sym.Type = static_cast<uint8_t>(global->type);
			sym.StorageClass = 2;
			sym.NumberOfAuxSymbols = 0;

			symbols.push_back(sym);
		}

		*size = sizeof(COFFFileHeader) +
			sizeof(SectionHeader) +
			(uint32_t)sizeCode +
			(sizeof(COFFSymbol) * (globals.size())) +
			stringTableSize;

		uint8_t* buffer = new uint8_t[*size];
		uint8_t* ptr = buffer;

		memcpy(ptr, &coffHeader, sizeof(COFFFileHeader));
		ptr += sizeof(COFFFileHeader);

		memcpy(ptr, &textHeader, sizeof(SectionHeader));
		ptr += sizeof(SectionHeader);

		memcpy(ptr, code, (uint32_t)sizeCode);
		ptr += (uint32_t)sizeCode;

		for (const auto& sym : symbols) {
			memcpy(ptr, &sym, sizeof(COFFSymbol));
			ptr += sizeof(COFFSymbol);
		}

		memcpy(ptr, &stringTableSize, sizeof(uint32_t));
		ptr += sizeof(uint32_t);

		memcpy(ptr, stringTable.data(), stringTableSize - sizeof(uint32_t));
		ptr += stringTableSize - sizeof(uint32_t);

		return buffer;
	}
}
