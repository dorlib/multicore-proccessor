#ifndef __PIPELINE_H__
#define __PIPELINE_H__

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "Cache.h"
#include "../../Interface/include/Helpers.h"
#include "Opcodes.h"

typedef enum {
	FETCH = 0,
	DECODE,
	EXECUTE,
	MEM,
	WRITE_BACK,

	PIPELINE_SIZE
} PipelineSM_e;

typedef struct {
	PipelineSM_e state;
	uint16_t pc;
	InstructionFormat_s instruction;
	uint32_t execute_result;
	void (*operation)(Opcode_function_params_s* params);
} PipelineStage_s;

typedef struct {
	uint32_t decode_stalls;
	uint32_t mem_stalls;
} PiplineStatistics_s;

typedef struct {
	bool halted;
	bool data_hazard_stall;
	bool memory_stall;
	uint32_t *insturcionts_p;
	uint32_t* core_registers_p;
	CacheData_s cache_data;
	PipelineStage_s pipe_stages[PIPELINE_SIZE];
	Opcode_function_params_s opcode_params;
	PiplineStatistics_s statistics;
} Pipeline_s;

void Pipeline_Init(Pipeline_s *pipeline);

void Pipeline_Execute(Pipeline_s* pipeline);

void Pipeline_WriteToTrace(Pipeline_s* pipeline, FILE *trace_file);

void Pipeline_BubbleCommands(Pipeline_s* pipeline);

bool Pipeline_PipeFlushed(Pipeline_s* pipeline);

#endif
