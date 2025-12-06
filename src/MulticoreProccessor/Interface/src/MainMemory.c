#define _CRT_SECURE_NO_WARNINGS

#include "../include/MainMemory.h"
#include "../include/Files.h"
#include "../include/Bus.h"
#include <string.h>


typedef union {
	uint32_t address;

	struct {
		uint16_t offset : 3;
		uint32_t block : 18;	
	} fields;
} memory_address_s;


static uint32_t gMemory[MAIN_MEMORY_SIZE];
static uint8_t counter;
static bool gMemoryTransaction;

static bool bus_transaction_handler(Bus_packet_s* packet, bool direct_transaction);
static void process_memory_word(Bus_packet_s* packet);
static uint32_t get_memory_length(void);

/**
 * @brief Initialize main memory from input file.
 */
void MainMemory_Init(void)
{
	memset((uint8_t*)gMemory, 0, sizeof(gMemory));
	counter = 0;
	gMemoryTransaction = false;

	uint16_t lineInProgram = 0;
	while (lineInProgram < MAIN_MEMORY_SIZE && fscanf(MeminFile, "%08x", (uint32_t*)&(gMemory[lineInProgram])) != EOF)
		lineInProgram++;

	Bus_RegisterMemoryCallback(bus_transaction_handler);
}

/**
 * @brief Print memory data to file.
 */
void MainMemory_PrintData(void) {
	uint32_t length = get_memory_length();
	for (uint32_t i = 0; i < length; i++)
		fprintf(MemoutFile, "%08X\n", gMemory[i]);
}

/**
 * @brief Direct write to memory (for cache flush at end of simulation).
 * 
 * @param address Memory address to write to.
 * @param data Data to write.
 */
void MainMemory_DirectWrite(uint32_t address, uint32_t data) {
	if (address < MAIN_MEMORY_SIZE) {
		gMemory[address] = data;
	}
}


/**
 * @brief Process a single memory word for bus transaction.
 *
 * @param[in,out] Bus_packet_s* packet - pointer to bus packet.
 */
static void process_memory_word(Bus_packet_s* packet) {
	uint32_t addr = packet->bus_addr & (MAIN_MEMORY_SIZE - 1);

	if (packet->bus_cmd == bus_busRd || packet->bus_cmd == bus_busRdX) {
		packet->bus_origid = bus_main_memory;
		packet->bus_cmd = bus_flush;
		packet->bus_data = gMemory[addr];
	} else if (packet->bus_cmd == bus_flush) {
		gMemory[addr] = packet->bus_data;
	}
}

/**
 * @brief Handle bus transactions to main memory.
 *
 * @param[in] Bus_packet_s* packet	 - pointer to bus packet.
 * @param[in] bool direct_transaction - is direct transaction to the memory.
 *
 * @return false if finished, true otherwise.
 */
static bool bus_transaction_handler(Bus_packet_s* packet, bool direct_transaction) {
	if (packet->bus_cmd == bus_no_command)
		return false;

	if (!gMemoryTransaction) {
		gMemoryTransaction = true;
		counter = !direct_transaction ? 0 : 16;
	}

	if (counter >= 16) {
		process_memory_word(packet);
		
		if (counter == 23)
			gMemoryTransaction = false;
		
		counter++;
		return true;
	}

	counter++;	
	return false;
}

static uint32_t get_memory_length(void) {
	uint32_t length = MAIN_MEMORY_SIZE - 1;

	for (; length > 0; length--) {
		if (gMemory[length])
			break;
	}

	return length + 1;
}
