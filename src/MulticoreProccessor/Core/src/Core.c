#define _CRT_SECURE_NO_WARNINGS
#include "../include/Core.h"
#include <stdio.h>
#include <string.h>

#define INSTRUCTION_COUNT 0

static int init_memory(Core_s* core);
static void write_trace(Core_s* core, uint32_t* regs_copy);
static void write_regs_to_file(Core_s* core, uint32_t* regs_copy);
static void update_statistics(Core_s* core);
static void print_register_file(Core_s* core);
static void print_statistics(Core_s* core);

/**
 * @brief Initialize the core.
 * 
 * @param core Pointer to the core structure.
 * @param id The identifier for the core.
 */
void Core_Init(Core_s *core, uint8_t id) {
	memset(&core->register_array, 0, sizeof(NUMBER_OF_REGISTERS));
	int number_of_lines = init_memory(core);
	if (!number_of_lines) {
		core->Core_isHalted = true;
		return;
	}

	core->program_counter = 0;
	core->index = id;
	core->Core_isHalted = false;

	memset(&core->statistics, 0, sizeof(Statistics_s));
	core->statistics.cycles = -1;

	memset(&core->pipeline, 0, sizeof(Pipeline_s));
	Pipeline_Init(&core->pipeline);

	memset(&core->pipeline.cache_data, 0, sizeof(CacheData_s));
	Cache_Init(&core->pipeline.cache_data, id);

	Cache_RegisterBusHandles();

	core->pipeline.core_registers_p = core->register_array;
	core->pipeline.insturcionts_p = core->instructions_memory_image;
	core->pipeline.opcode_params.pc = &(core->program_counter);
}

/**
 * @brief Run core iteration
 * 
 * @param core Pointer to the core structure.
 */
void Core_Iter(Core_s* core) {
	if (core->Core_isHalted) {
		return;
	}

	if (Pipeline_PipeFlushed(&core->pipeline)) {
		core->Core_isHalted = true;
		return;
	}

	uint32_t regs_copy[NUMBER_OF_REGISTERS];
	memcpy(regs_copy, core->register_array, sizeof(core->register_array));

	update_statistics(core);
	Pipeline_Execute(&core->pipeline);
	write_trace(core, regs_copy);
	Pipeline_BubbleCommands(&core->pipeline);
}

/**
 * @brief Teardown of the core.
 * 
 * @param core Pointer to the core structure.
 */
void Core_Teaddown(Core_s* core) {
	Cache_FlushToMemory(&core->pipeline.cache_data);
	print_register_file(core);
	Cache_PrintData(&core->pipeline.cache_data,
		core->core_files.DsRamFile, core->core_files.TsRamFile);
	print_statistics(core);
}

/**
 * @brief Check if the core is halted.
 * 
 * @param core Pointer to the core structure.
 * @return true if core is halted, fale otherwise.
 */
bool Core_isHalted(Core_s* core) {
	return core->Core_isHalted;
}

/**
 * @brief Initialize the instruction memory of the core.
 * 
 * @param core Pointer to the core structure.
 * @return The number of instructions loaded into memory.
 */
static int init_memory(Core_s* core) {
	int number_of_lines = 0; 
	while (number_of_lines < INSTRUCTIONS_MEMORY_SIZE && fscanf(core->core_files.InstructionMemFile,
		"%08x", (uint32_t*)&(core->instructions_memory_image[number_of_lines])) != EOF) {
		number_of_lines++;
	}

	return number_of_lines;
}

/**
 * @brief Write the trace of the core.
 * 
 * @param core Pointer to the operating core
 * @param regs_copy Pointer to the regs copied values
 */
static void write_trace(Core_s* core, uint32_t *regs_copy) {
	fprintf(core->core_files.TraceFile, "%d ", core->statistics.cycles);
	Pipeline_WriteToTrace(&core->pipeline, core->core_files.TraceFile);
	write_regs_to_file(core, regs_copy);
	fprintf(core->core_files.TraceFile, "\n");

}

/**
 * @brief Write the registers to file.
 * 
 * @param core Pointer to the operating core.
 * @param regs_copy Pointer to the regs copied values.
 */
static void write_regs_to_file(Core_s* core, uint32_t* regs_copy) {
	for (int i = START_MUTABLE_REGISTER_INDEX; i < NUMBER_OF_REGISTERS; i++) {
		fprintf(core->core_files.TraceFile, "%08X ", regs_copy[i]);
	}
}

/**
 * @brief Update core statistics.
 * 
 * @param core Pointer to the operating core.
 */
static void update_statistics(Core_s* core) {
	core->statistics.cycles++;

	if (!core->pipeline.halted && !core->pipeline.memory_stall && !core->pipeline.data_hazard_stall)
		core->statistics.instructions++;
}

/**
 * @brief Print the register file to the register file.
 * 
 * @param core Pointer to the operating core.
 */
static void print_register_file(Core_s* core) {
	for (int i = START_MUTABLE_REGISTER_INDEX; i < NUMBER_OF_REGISTERS; i++) {
		fprintf(core->core_files.RegFile, "%08X\n", core->register_array[i]);
	}
}

/**
 * @brief Print the core statistics to the stats file.
 * 
 * @param core Pointer to the operating core.
 */
static void print_statistics(Core_s* core) {
	fprintf(core->core_files.StatsFile, "cycles %d\n", core->statistics.cycles + 1);
	fprintf(core->core_files.StatsFile, "instructions %d\n", core->statistics.instructions - 1);
	fprintf(core->core_files.StatsFile, "read_hit %d\n", core->pipeline.cache_data.statistics.read_hits);
	fprintf(core->core_files.StatsFile, "write_hit %d\n", core->pipeline.cache_data.statistics.write_hits);
	fprintf(core->core_files.StatsFile, "read_miss %d\n", core->pipeline.cache_data.statistics.read_misses);
	fprintf(core->core_files.StatsFile, "write_miss %d\n", core->pipeline.cache_data.statistics.write_misses);
	fprintf(core->core_files.StatsFile, "decode stall %d\n", core->pipeline.statistics.decode_stalls);
	fprintf(core->core_files.StatsFile, "mem stall %d\n", core->pipeline.statistics.mem_stalls);
}
