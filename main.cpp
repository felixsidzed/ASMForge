#include <iomanip>
#include <iostream>
#include <fstream>

#include "ASMForge/forge.h"

uint8_t* forge(ASMForge::ExtendedModuleBuilder* M, size_t& size) {
	using namespace ASMForge;

	SubRoutineBuilder* func = M->CreateSubRoutine("_main");
	func->Append(func->Create(OpCode::MOV, Reg::AX, 5));
	func->Append(func->Create(OpCode::MOV, Reg::BX, 5));
	func->Append(func->Create(OpCode::ADD, Reg::AX, Reg::BX));

	M->MakeGlobal(func);
	return M->EmitCOFF(&size);
}

int main() {
	using namespace ASMForge;
	ASMForge::ExtendedModuleBuilder* M = ASMForge::New("Test");

	size_t size;
	uint8_t* coff = forge(M, size);

	std::cout << "=== Module '" << M->name << "' (" << size << " bytes) ===\n    ";

	std::cout << std::hex << std::setfill('0');
	for (size_t i = 0; i < size; ++i) {
		std::cout << std::setw(2) << (static_cast<uint16_t>(coff[i])) << " ";
		if ((i + 1) % 16 == 0) {
			std::cout << "\n    ";
		}
	}
	if (size % 16 != 0) {
		std::cout << "\n    ";
	}

	std::cout << std::dec << std::endl;
	
	std::ofstream out(M->name + ".o", std::ios::binary);
	if (out) {
		out.write(reinterpret_cast<char*>(coff), size);
		out.close();
		std::cout << "COFF written to '" << M->name << ".o'" << std::endl;
	} else {
		std::cerr << "Failed to write to file" << std::endl;;
	}

	delete M;
	delete[] coff;

	return 0;
}
