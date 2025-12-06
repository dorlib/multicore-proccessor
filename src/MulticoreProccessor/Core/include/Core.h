#ifndef CORE_H_
#define CORE_H_

#include <stdint.h>
#include "Pipeline.h"
#include "../../Interface/include/Helpers.h"
#include "../../Interface/include/Files.h"

#define INSTRUCTIONS_MEMORY_SIZE 1024

typedef struct { 
	uint32_t cycles;
	uint32_t instructions;
} Statistics_s;

typedef struct {
	bool Core_isHalted;
	uint8_t index;
	uint16_t program_counter;	// pc is 10bit
	uint32_t register_array[NUMBER_OF_REGISTERS];
	uint32_t instructions_memory_image[INSTRUCTIONS_MEMORY_SIZE];				
	Core_Files core_files;
	Pipeline_s pipeline;
	Statistics_s statistics;
} Core_s;


void Core_Init(Core_s* core, uint8_t id);

void Core_Iter(Core_s* core);

void Core_Teaddown(Core_s* core);

bool Core_isHalted(Core_s* core);

#endif
