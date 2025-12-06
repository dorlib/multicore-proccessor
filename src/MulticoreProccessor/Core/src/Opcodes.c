#include "../include/Opcodes.h"

#define _CSECURE_NO_WARNINGS

/**
 * @brief R[params->rd] = R[params->rs] + R[params->rt]
 * 
 * @param params Pointer to the opcode function parameters struct.
 */
void Add(Opcode_function_params_s *params) {
	*params->rd = params->rs + params->rt;
}

/**
 * @brief R[params->rd] = R[params->rs] - R[params->rt]
 * 
 * @param params Pointer to the opcode function parameters struct.
 */
void Sub(Opcode_function_params_s *params) {
	*params->rd = params->rs - params->rt;
}

/**
 * @brief R[params->rd] = R[params->rs] & R[params->rt]
 * 
 * @param params Pointer to the opcode function parameters struct.
 */
void And(Opcode_function_params_s *params) {
	*params->rd = params->rs & params->rt;
}

/**
 * @brief R[params->rd] = R[params->rs] | R[params->rt]
 * 
 * @param params Pointer to the opcode function parameters struct.
 */
void Or(Opcode_function_params_s *params) {
	*params->rd = params->rs | params->rt;
}

/**
 * @brief R[params->rd] = R[params->rs] ^ R[params->rt]
 * 
 * @param params Pointer to the opcode function parameters struct.
 */
void Xor(Opcode_function_params_s *params) {
	*params->rd = params->rs ^ params->rt;
}

/**
 * @brief R[params->rd] = R[params->rs] * R[params->rt]
 * 
 * @param params Pointer to the opcode function parameters struct.
 */
void Multiply(Opcode_function_params_s *params) {
	*params->rd = params->rs * params->rt;
}

/**
 * @brief R[params->rd] = R[params->rs] << R[params->rt]
 * 
 * @param params Pointer to the opcode function parameters struct.
 */
void LogicShiftLeft(Opcode_function_params_s *params) {
	*params->rd = params->rs << params->rt;
}

/**
 * @brief R[params->rd] = R[params->rs] >> R[params->rt] (arithmetic shift)
 * 
 * @param params Pointer to the opcode function parameters struct.
 */
void ArithmeticShiftRight(Opcode_function_params_s *params) {
	*params->rd = (int)params->rs >> (int)params->rt;
} 

/**
 * @brief R[params->rd] = R[params->rs] >> R[params->rt] (logical shift)
 * 
 * @param params Pointer to the opcode function parameters struct.
 */
void LogicShiftRight(Opcode_function_params_s *params) {
	*params->rd = params->rs >> params->rt;
}

/**
 * @brief if (R[params->rs] == R[params->rt]) pc = R[params->rd][low bits 9:0]
 * 
 * @param params Pointer to the opcode function parameters struct.
 */
void BranchEqual(Opcode_function_params_s *params) {
	if (params->rs == params->rt) {
		*params->pc = (uint16_t)(*params->rd & 0x1FF);
	}
}

/**
 * @brief if (R[params->rs] != R[params->rt]) pc = R[params->rd][low bits 9:0]
 * 
 * @param params Pointer to the opcode function parameters struct. 
 */
void BranchNotEqual(Opcode_function_params_s* params) {
	if (params->rs != params->rt) {
		*params->pc = (uint16_t)(*params->rd & 0x1FF);
	}
}

/**
 * @brief if (R[params->rs] < R[params->rt]) pc = R[params->rd][low bits 9:0]
 * 
 * @param params Pointer to the opcode function parameters struct.
 */
void BranchLessThen(Opcode_function_params_s* params) {
	if (params->rs < params->rt) {
		*params->pc = (uint16_t)(*params->rd & 0x1FF);
	}
}

/**
 * @brief if (R[params->rs] > R[params->rt]) pc = R[params->rd][low bits 9:0]
 * 
 * @param params Pointer to the opcode function parameters struct.
 */
void BranchGreaterThan(Opcode_function_params_s* params) {
	if (params->rs > params->rt) {
		*params->pc = (uint16_t)(*params->rd & 0x1FF);
	}
}

/**
 * @brief if (R[params->rs] <= R[params->rt]) pc = R[params->rd][low bits 9:0]
 * 
 * @param params Pointer to the opcode function parameters struct.
 */
void BranchLessEqual(Opcode_function_params_s *params) {
	if (params->rs <= params->rt) {
		*params->pc = (uint16_t)(*params->rd & 0x1FF);
	}
}

/**
 * @brief if (R[params->rs] >= R[params->rt]) pc = R[params->rd][low bits 9:0]
 * 
 * @param params Pointer to the opcode function parameters struct.
 */
void BranchGraterEqual(Opcode_function_params_s *params) {
	if (params->rs >= params->rt) {
		*params->pc = (uint16_t)(*params->rd & 0x1FF);
	}
}

/**
 * @brief R[params->rd] = pc; pc = R[params->rd][low bits 9:0]
 * 
 * @param params Pointer to the opcode function parameters struct.
 */
void JumpAndLink(Opcode_function_params_s *params) {
	*params->rd = *params->pc;
	*params->pc = (uint16_t)(*params->rd & 0x1FF);
}

/**
 * @brief Check if the opcode is of baranch resulotion.
 * 
 * @param opcode The opcode we are testing.
 * @return true if thie is branch resulotion, false otherwise.
 */
bool Opcode_IsBranchResulotion(uint16_t opcode) {
	return opcode >= BEQ && opcode < LW;
}

/**
 * @brief Check if the opcode is of memory command.
 * 
 * @param opcode The opcode we are testing.
 * @return true if thie is memory command, false otherwise.
 */
bool Opcode_IsMemoryCommand(uint16_t opcode) {
	return opcode == LW || opcode == SW;
}
