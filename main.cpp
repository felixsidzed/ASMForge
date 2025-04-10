#include <iomanip>
#include <iostream>
#include <fstream>

#include "ASMForge/forge.h"

int main() {
	ASMForge::ModuleBuilder* M = ASMForge::New("Test", false);

	ASMForge::SubRoutineBuilder* func = M->CreateSubRoutine("main");
	func->Append(func->Create(ASMForge::OpCode::MOV, ASMForge::Reg::AX, 6969));
	func->Append(func->Create(ASMForge::OpCode::RET));

	size_t size;
	uint8_t* code = M->EmitCOFF(&size);

	std::cout << "=== Module '" << M->name << "' (" << size << " bytes) ===\n    ";

	std::cout << std::hex << std::setfill('0');
	for (size_t i = 0; i < size; ++i) {
		std::cout << std::setw(2) << (static_cast<uint16_t>(code[i])) << " ";
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
		out.write(reinterpret_cast<char*>(code), size);
		out.close();
		std::cout << "COFF written to '" << M->name << ".o'" << std::endl;
	} else {
		std::cerr << "Failed to write to file" << std::endl;;
	}

	delete M;
	delete[] code;

	return 0;
}
