#ifndef OPCODES
#define OPCODES

#include <stdint.h>
#include "../../Interface/include/Helpers.h"

#define NUMBER_OF_OPCODES 21

typedef enum {
	ADD = 0,
	SUB,
	AND,
	OR,
	XOR,
	MUL,
	SLL,
	SRA,
	SRL,
	BEQ,
	BNE,
	BLT,
	BGT,
	BLE,
	BGE,
	JAL,
	LW,
	SW,
	HALT = 20 
} OpcodeOperations;

typedef struct {
	uint32_t *rd;
	uint32_t rs;
	uint32_t rt;
	uint32_t* memory_p;
	uint16_t *pc;		
	bool* halt;
} Opcode_function_params_s;

void Add(Opcode_function_params_s* params);
void Sub(Opcode_function_params_s* params);
void And(Opcode_function_params_s* params);
void Or(Opcode_function_params_s* params);
void Xor(Opcode_function_params_s* params);
void Multiply(Opcode_function_params_s* params);
void LogicShiftLeft(Opcode_function_params_s* params);
void ArithmeticShiftRight(Opcode_function_params_s* params);
void LogicShiftRight(Opcode_function_params_s* params);
void BranchEqual(Opcode_function_params_s* params);
void BranchNotEqual(Opcode_function_params_s* params);
void BranchLessThen(Opcode_function_params_s* params);
void BranchGreaterThan(Opcode_function_params_s* params);
void BranchLessEqual(Opcode_function_params_s* params);
void BranchGraterEqual(Opcode_function_params_s* params);
void JumpAndLink(Opcode_function_params_s* params);

static void (*OpcodeMapping[NUMBER_OF_OPCODES])(Opcode_function_params_s* params) = {
	 Add, Sub, And, Or, Xor, Multiply, LogicShiftLeft, ArithmeticShiftRight,
	 LogicShiftRight, BranchEqual, BranchNotEqual, BranchLessThen, BranchGreaterThan,
	 BranchLessEqual, BranchGraterEqual, JumpAndLink
};

bool Opcode_IsBranchResulotion(uint16_t opcode);

bool Opcode_IsMemoryCommand(uint16_t opcode);

#endif __OPCODES
