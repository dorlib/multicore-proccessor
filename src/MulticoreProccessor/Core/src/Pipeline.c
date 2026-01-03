#include "..\include\Pipeline.h"
#include <string.h>
#include "..\include\Cache.h"

static void fetch(Pipeline_s* pipeline);
static void decode(Pipeline_s* pipeline);
static void execute(Pipeline_s* pipeline);
static void mem(Pipeline_s* pipeline);
static void writeback(Pipeline_s* pipeline);
static void execute_stages(Pipeline_s* pipeline);
static void prepare_registers_params(Pipeline_s* pipeline, PipelineSM_e stage);
static bool is_pipeline_needs_data_hazard_stall(Pipeline_s* pipeline);
static bool compare_register(Pipeline_s* pipeline, uint16_t reg, uint16_t stage);
static bool check_registers_hazrads(Pipeline_s* pipeline, PipelineSM_e stage);
static void (*pipe_functions[PIPELINE_SIZE])(Pipeline_s* pipeline) = {
	fetch, decode, execute, mem, writeback
};
static void update_statistics(Pipeline_s* pipeline);

/**
 * @brief Initialize the pipeline
 * 
 * @param pipeline Pointer to the relevant pipeline.
 */
void Pipeline_Init(Pipeline_s *pipeline) {
	pipeline->halted = false;
	pipeline->data_hazard_stall = false;
	pipeline->memory_stall = false;
	
	memset((uint8_t *)&pipeline->statistics, 0, sizeof(pipeline->statistics));
	memset((uint8_t *) &pipeline->opcode_params, 0, sizeof(pipeline->opcode_params));
	pipeline->opcode_params.halt = &pipeline->halted;

	memset((uint8_t*) pipeline->pipe_stages, 0, sizeof(pipeline->pipe_stages));
	for (int stage = FETCH; stage < PIPELINE_SIZE; stage++) {
		pipeline->pipe_stages[stage].state = stage;
		pipeline->pipe_stages[stage].pc = UINT16_MAX;
	}

	pipeline->pipe_stages[FETCH].pc = 0;
}

/**
 * @brief Execute the pipeline.
 * 
 * @param pipeline Pointer to the relevant pipeline.
 */
void Pipeline_Execute(Pipeline_s* pipeline) {
	pipeline->data_hazard_stall = is_pipeline_needs_data_hazard_stall(pipeline);
	execute_stages(pipeline);
	update_statistics(pipeline);
}

/**
 * @brief Check if the pipeline has flushed all stages.
 * 
 * @param pipeline Pointer to the relevant pipeline.
 * @return true if the pipeline is flushed, false otherwise.
 */
bool Pipeline_PipeFlushed(Pipeline_s* pipeline)
{
	bool flushed = pipeline->halted;
	for (int stage = FETCH; stage < PIPELINE_SIZE; stage++)
	{
		flushed &= (pipeline->pipe_stages[stage].pc == UINT16_MAX);
	}

	return flushed;
}

/**
 * @brief Write the pipeline state to the trace file.
 * 
 * @param pipeline Pointer to the relevant pipeline.
 * @param trace_file Pointer to the trace file.
 */
void Pipeline_WriteToTrace(Pipeline_s* pipeline, FILE* trace_file) {
	for (int stage = FETCH; stage < PIPELINE_SIZE; stage++) {
		if (pipeline->pipe_stages[stage].pc == UINT16_MAX)
			fprintf(trace_file, "--- ");
		else
			fprintf(trace_file, "%03X ", pipeline->pipe_stages[stage].pc);
	}
}

/**
 * @brief Bubble the commands in the pipeline based on stalls.
 * 
 * @param pipeline Pointer to the relevant pipeline.
 */
void Pipeline_BubbleCommands(Pipeline_s* pipeline) {
	for (int stage = PIPELINE_SIZE - 1; stage > FETCH; stage--) {
		if (pipeline->memory_stall) {
			pipeline->pipe_stages[WRITE_BACK].pc = UINT16_MAX;
			break;
		} else if (pipeline->data_hazard_stall && stage == EXECUTE) {
			pipeline->pipe_stages[EXECUTE].pc = UINT16_MAX;
			break;
		} else if (pipeline->pipe_stages[stage - 1].pc == UINT16_MAX) {
			pipeline->pipe_stages[stage].pc = UINT16_MAX;
		} else {
			pipeline->pipe_stages[stage].pc = pipeline->pipe_stages[stage - 1].pc;
			pipeline->pipe_stages[stage].instruction.command = pipeline->pipe_stages[stage - 1].instruction.command;
			pipeline->pipe_stages[stage].operation = *pipeline->pipe_stages[stage - 1].operation;
			pipeline->pipe_stages[stage].execute_result = pipeline->pipe_stages[stage - 1].execute_result;
		}
	}

	if (pipeline->halted) {
		pipeline->pipe_stages[FETCH].pc = UINT16_MAX;
		pipeline->pipe_stages[DECODE].pc = UINT16_MAX;
	}
}

/** 
 * @brief Fetch stage of the pipeline.
 * 
 * @param pipeline Pointer to the relevant pipeline.
 */
static void fetch(Pipeline_s* pipeline) {
	if(pipeline->memory_stall) {
		return;
	}

	pipeline->pipe_stages[FETCH].pc = *(pipeline->opcode_params.pc);
	pipeline->pipe_stages[FETCH].instruction.command = pipeline->insturcionts_p[*(pipeline->opcode_params.pc)];
	if (!pipeline->data_hazard_stall) {
		*(pipeline->opcode_params.pc)+= 1;
	}
}

/** 
 * @brief Decode stage of the pipeline.
 * 
 * @param pipeline Pointer to the relevant pipeline.
 */
static void decode(Pipeline_s* pipeline) {
	uint16_t opcode = pipeline->pipe_stages[DECODE].instruction.bits.opcode;
	if (opcode == HALT) {
		pipeline->halted = true;
		return;
	}

	pipeline->pipe_stages[DECODE].operation = OpcodeMapping[opcode];

	if (Opcode_IsBranchResulotion(pipeline->pipe_stages[DECODE].instruction.bits.opcode)) {
		prepare_registers_params(pipeline, DECODE);
		pipeline->pipe_stages[DECODE].operation(&pipeline->opcode_params);
	}
}

static void execute(Pipeline_s* pipeline) {
	uint16_t opcode = pipeline->pipe_stages[EXECUTE].instruction.bits.opcode;
	if (!Opcode_IsBranchResulotion(opcode) && !Opcode_IsMemoryCommand(opcode) && opcode != HALT) {
		prepare_registers_params(pipeline, EXECUTE);
		pipeline->pipe_stages[EXECUTE].operation(&pipeline->opcode_params);
	}
}

/** 
 * @brief Memory stage of the pipeline.
 * 
 * @param pipeline Pointer to the relevant pipeline.
 */
static void mem(Pipeline_s* pipeline) {
	uint16_t opcode = pipeline->pipe_stages[MEM].instruction.bits.opcode;
 	if (Opcode_IsMemoryCommand(opcode)) {
		prepare_registers_params(pipeline, MEM);
		uint32_t address = pipeline->opcode_params.rs + pipeline->opcode_params.rt;

		uint32_t* data = pipeline->opcode_params.rd;
		bool response = opcode == LW ? Cache_ReadData(&pipeline->cache_data, address, data) :
			Cache_WriteData(&pipeline->cache_data, address, *data);

		pipeline->memory_stall = !response;
	}
}

/** 
 * @brief Writeback stage of the pipeline.
 * 
 * @param pipeline Pointer to the relevant pipeline.
 */
static void writeback(Pipeline_s* pipeline) {
	InstructionFormat_s instuction = { .command = pipeline->pipe_stages[WRITE_BACK].instruction.command };
	int index = instuction.bits.opcode == JAL ? PROGRAM_COUNTER_REGISTER_NUM : instuction.bits.rd;
	pipeline->core_registers_p[index] = pipeline->pipe_stages[WRITE_BACK].execute_result;
}

/**
 * @brief Prepare the registers parameters for the opcode execution.
 * 
 * @param pipeline Pointer to the relevant pipeline.
 * @param stage The pipeline stage we are preparing the params for.
 */
static void prepare_registers_params(Pipeline_s *pipeline, PipelineSM_e stage) {
	InstructionFormat_s instuction = {.command = pipeline->pipe_stages[stage].instruction.command};
	pipeline->core_registers_p[IMMEDIATE_REGISTER_INDEX] = instuction.bits.immediate;
	pipeline->pipe_stages[stage].execute_result = pipeline->core_registers_p[instuction.bits.rd];

	pipeline->opcode_params.rs = pipeline->core_registers_p[instuction.bits.rs];
	pipeline->opcode_params.rt = pipeline->core_registers_p[instuction.bits.rt];
	pipeline->opcode_params.rd = &pipeline->pipe_stages[stage].execute_result;
}

/**
 * @brief Execute the stages of the pipeline based on stalls.
 * 
 * @param pipeline Pointer to the relevant pipeline.
 */
static void execute_stages(Pipeline_s* pipeline) {
	uint8_t stage = pipeline->memory_stall ? MEM : pipeline->data_hazard_stall ? EXECUTE : DECODE;
	if(!pipeline->halted)
		pipe_functions[FETCH](pipeline);

	for (; stage < PIPELINE_SIZE; stage++) {
		if (!(pipeline->pipe_stages[stage].pc == UINT16_MAX)) {
			pipe_functions[stage](pipeline);
		}
	}
}

/**
 * @brief Compare the registers of a certain stage with a given register.
 * 
 * @param pipeline Pointer to the relevant pipeline.
 * @param reg The register to compare to.
 * @param stage The pipeline stage to compare.
 * @return true if two identical register indexes are found, false otherwise.
 */
static bool compare_register(Pipeline_s* pipeline, uint16_t reg, uint16_t stage)
{
	bool ret = false;
	InstructionFormat_s decode_ins = pipeline->pipe_stages[DECODE].instruction;
	uint16_t op = pipeline->pipe_stages[stage].instruction.bits.opcode;

	if (reg == IMMEDIATE_REGISTER_INDEX || reg == ZERO_REGISTER_INDEX) {
		ret = false;
	} else if (decode_ins.bits.opcode <= SRL || decode_ins.bits.opcode == LW) {
		// ALU ops and LW use rs and rt as sources only
		ret = (reg == decode_ins.bits.rs || reg == decode_ins.bits.rt);
	} else if (decode_ins.bits.opcode == SW) {
		// SW uses rd, rs, rt all as sources (rd is the data to store)
		ret = (reg == decode_ins.bits.rd || reg == decode_ins.bits.rs || reg == decode_ins.bits.rt);
	} else {
		// Branch instructions use rd, rs, rt
		ret = (reg == decode_ins.bits.rd || reg == decode_ins.bits.rs || reg == decode_ins.bits.rt);
	}

	return ret;
}

/**
 * @brief Checking registers hazards.
 * 
 * @param pipeline Pointer to the relevant pipeline.
 * @param stage The pipeline stage to compare.
 * @return true if found hazard, false otherwise.
 */
static bool check_registers_hazrads(Pipeline_s *pipeline, PipelineSM_e stage) {
	if (pipeline->pipe_stages[stage].pc == UINT16_MAX) {
		return false;
	}

	// Skip hazard check for instructions that don't write to a register:
	uint16_t stage_opcode = pipeline->pipe_stages[stage].instruction.bits.opcode;
	if (stage_opcode == SW || Opcode_IsBranchResulotion(stage_opcode) || stage_opcode == HALT) {
		return false;
	}

	return compare_register(pipeline, pipeline->pipe_stages[stage].instruction.bits.rd, stage);
}

/**
 * @brief Check if the pipeline needs data hazard stall.
 * 
 * @param pipeline Pointer to the relevant pipeline.
 * @return true if data hazard stall is needed, false otherwise.
 */
static bool is_pipeline_needs_data_hazard_stall(Pipeline_s* pipeline) {
	return check_registers_hazrads(pipeline, EXECUTE) || check_registers_hazrads(pipeline, WRITE_BACK) ||
		check_registers_hazrads(pipeline, MEM);
}

/**
 * @brief Update the pipeline statistics.
 * 
 * @param pipeline Pointer to the relevant pipeline.
 */
static void update_statistics(Pipeline_s* pipeline) {
	if (pipeline->data_hazard_stall && !pipeline->memory_stall)
		pipeline->statistics.decode_stalls++;

	if (pipeline->memory_stall)
		pipeline->statistics.mem_stalls++;
}

