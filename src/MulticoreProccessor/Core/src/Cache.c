#include "..\include\Cache.h"
#include <string.h>
#include "..\..\Interface\include\Bus.h"
#include "..\..\Interface\include\MainMemory.h"

typedef union {
	uint32_t address;

	struct {
		uint32_t offset : 3;
		uint32_t index : 6;		
		uint32_t tag : 12;		
	} fields;
} cache_addess_s;

typedef Cache_mesi_e(*snooping_state)(CacheData_s* data, Bus_packet_s* packet);

static void dirty_block_handling(CacheData_s* data, cache_addess_s addr);

static bool shared_signal_handle(CacheData_s* data, Bus_packet_s* packet, bool* is_modified);
static bool cache_snooping_handle(CacheData_s* data, Bus_packet_s* packet, uint8_t address_offset);
static bool cache_response_handle(CacheData_s* data, Bus_packet_s* packet, uint8_t* address_offset);

static Cache_mesi_e mesi_snooping_invalid_state(CacheData_s* data, Bus_packet_s* packet);
static Cache_mesi_e mesi_snooping_shared_state(CacheData_s* data, Bus_packet_s* packet);
static Cache_mesi_e mesi_snooping_exclusive_state(CacheData_s* data, Bus_packet_s* packet);
static Cache_mesi_e mesi_snooping_modified_state(CacheData_s* data, Bus_packet_s* packet);

static snooping_state gSnoopingSM[cache_mesi_max_state] = {
	mesi_snooping_invalid_state,
	mesi_snooping_shared_state,
	mesi_snooping_exclusive_state,
	mesi_snooping_modified_state
};

/**
 * @brief Initialize the cache data structure.
 * 
 * @param data Pointer to the cache data structure.
 * @param id Identifier for the cache.
 */
void Cache_Init(CacheData_s* data, Cache_Id_e id) {
	memset((uint8_t*)data, 0, sizeof(*data)); 
	data->id = id;

	Bus_cache_interface_s cache_interface = { .core_id = id, .cache_data = data };
	Bus_RegisterCache(cache_interface);
}

/**
 * @brief Register bus callbacks for cache operations.
 */
void Cache_RegisterBusHandles(void) {
	Bus_RegisterCacheCallbacks(shared_signal_handle, cache_snooping_handle, cache_response_handle);
}

/**
 * @brief Read data from the cache.
 * 
 * @param cache_data Pointer to the cache data structure.
 * @param address Address to read from.
 * @param data Pointer to store the read data.
 * 
 * @return true if read was successful, false otherwise.
 */
bool Cache_ReadData(CacheData_s* cache_data, uint32_t address, uint32_t* data) {
	static bool miss_occurred = false;

	if (Bus_InTransaction(cache_data->id) || Bus_WaitForTransaction(cache_data->id))
		return false;
	
	cache_addess_s addr;
	addr.address = address;
	Tsram_s* tsram = &(cache_data->tsram[addr.fields.index]);
	 
	if (tsram->fields.tag == addr.fields.tag && tsram->fields.mesi != cache_mesi_invalid) {
		uint16_t index = addr.fields.index * BLOCK_SIZE + addr.fields.offset;
		*data = cache_data->dsram[index];

		if (!miss_occurred)
			cache_data->statistics.read_hits++;
		else
			miss_occurred = false;

		return true;
	}

	miss_occurred = true;
	cache_data->statistics.read_misses++;

	dirty_block_handling(cache_data, addr);

	Bus_packet_s packet = {
		.bus_origid = cache_data->id, .bus_cmd = bus_busRd, .bus_addr = addr.address, .bus_data = 0, .bus_shared = 0 };

	Bus_AddTransaction(packet);

	return false;
}

/**
 * @brief Write data to the cache.
 * 
 * @param cache_data Pointer to the cache data structure.
 * @param address Address to write to.
 * @param data Data to write.
 * 
 * @return true if write was successful, false otherwise.
 */
bool Cache_WriteData(CacheData_s* cache_data, uint32_t address, uint32_t data) {
	static bool miss_occurred = false;
	
	if (Bus_InTransaction(cache_data->id) || Bus_WaitForTransaction(cache_data->id))
		return false; 
	
	cache_addess_s addr;
	addr.address = address;
	Tsram_s* tsram = &(cache_data->tsram[addr.fields.index]);
	
	if (tsram->fields.tag == addr.fields.tag && tsram->fields.mesi != cache_mesi_invalid) {
		if (tsram->fields.mesi == cache_mesi_shared) {
			miss_occurred = true;
			cache_data->statistics.write_misses++;

			Bus_packet_s packet = {
				.bus_origid = cache_data->id, .bus_cmd = bus_busRdX, .bus_addr = addr.address, .bus_data = 0, .bus_shared = 0 };

			Bus_AddTransaction(packet);

			Bus_packet_s invalid_packet = { .bus_origid = bus_invalid_originator };
			Bus_AddTransaction(invalid_packet);
			return false;
		}

		if (!miss_occurred)
			cache_data->statistics.write_hits++;
		else
			miss_occurred = false;

		uint16_t index = addr.fields.index * BLOCK_SIZE + addr.fields.offset;
		cache_data->dsram[index] = data;
		cache_data->tsram[addr.fields.index].fields.mesi = cache_mesi_modified;
		return true;
	}

	miss_occurred = true;
	cache_data->statistics.write_misses++;

	dirty_block_handling(cache_data, addr);

	Bus_packet_s packet = {
		.bus_origid = cache_data->id, .bus_cmd = bus_busRdX, .bus_addr = addr.address, .bus_data = 0, .bus_shared = 0 };

	Bus_AddTransaction(packet);
	
	return false;
}

/**
 * @brief Print the cache data to the provided files.
 * 
 * @param cache_data Pointer to the cache data structure.
 * @param dsram_file File pointer for DSRAM output.
 * @param tsram_file File pointer for TSRAM output.
 */
void Cache_PrintData(CacheData_s* cache_data, FILE* dsram_file, FILE* tsram_file) {
	for (uint32_t i = 0; i < CACHE_SIZE; i++)
		fprintf(dsram_file, "%08X\n", cache_data->dsram[i]);

	for (uint32_t i = 0; i < FRAME_SIZE; i++)
		fprintf(tsram_file, "%08X\n", cache_data->tsram[i].data);
}

/**
 * @brief Flush all dirty (modified) cache blocks to main memory.
 * 
 * This function is called at the end of simulation to ensure all
 * modified data is written back to main memory before outputting memout.txt.
 * 
 * @param cache_data Pointer to the cache data structure.
 */
void Cache_FlushToMemory(CacheData_s* cache_data) {
	for (uint32_t frame = 0; frame < FRAME_SIZE; frame++) {
		if (cache_data->tsram[frame].fields.mesi == cache_mesi_modified) {
			// Reconstruct the base address of this cache block
			uint32_t tag = cache_data->tsram[frame].fields.tag;
			uint32_t base_addr = (tag << 9) | (frame << 3);  // tag:12, index:6, offset:3
			
			// Write all 8 words of the block to main memory
			for (uint32_t offset = 0; offset < BLOCK_SIZE; offset++) {
				uint32_t dsram_index = frame * BLOCK_SIZE + offset;
				uint32_t mem_addr = base_addr + offset;
				MainMemory_DirectWrite(mem_addr, cache_data->dsram[dsram_index]);
			}
		}
	}
}


/**
 * @brief Handle dirty block by writing it back to memory.
 * 
 * @param data Pointer to the cache data structure.
 * @param addr Address of the block to handle.
 */
static void dirty_block_handling(CacheData_s* data, cache_addess_s addr) {
	if (data->tsram[addr.fields.index].fields.mesi == cache_mesi_modified) {
		cache_addess_s block_addr = {
			.fields.index = addr.fields.index,
			.fields.tag = data->tsram[addr.fields.index].fields.tag,
			.fields.offset = 0
		};

		Bus_packet_s packet = {
			.bus_origid = data->id, .bus_cmd = bus_flush, .bus_addr = block_addr.address, .bus_shared = 0 };

		uint16_t index = addr.fields.index * BLOCK_SIZE + addr.fields.offset;
		packet.bus_data = data->dsram[index];

		Bus_AddTransaction(packet);
	}
}


/**
 * @brief Handle shared signal on the bus.
 * 
 * @param data Pointer to the cache data structure.
 * @param packet Pointer to the bus packet.
 * @param is_modified Pointer to the is_modified flag.
 * @return true if shared line is set, false otherwise.
 */
static bool shared_signal_handle(CacheData_s* data, Bus_packet_s* packet, bool* is_modified){
	if (data->id == packet->bus_origid)
		return false;

	cache_addess_s address = { .address = packet->bus_addr };
	Tsram_s* tsram = &(data->tsram[address.fields.index]);
	*is_modified |= tsram->fields.mesi == cache_mesi_modified;
	return tsram->fields.tag == address.fields.tag && tsram->fields.mesi != cache_mesi_invalid;
}

/**
 * @brief Handle cache snooping on the bus.
 * 
 * @param data Pointer to the cache data structure.
 * @param packet Pointer to the bus packet.
 * @param address_offset Current address offset.
 * @return true if any cache responded, false otherwise.
 */
static bool cache_snooping_handle(CacheData_s* data, Bus_packet_s* packet, uint8_t address_offset) {
	if (data->id == packet->bus_original_sender && packet->bus_cmd != bus_flush)
		return false;

	cache_addess_s address = { .address = packet->bus_addr };
	Tsram_s* tsram = &(data->tsram[address.fields.index]);

	if (tsram->fields.tag != address.fields.tag || tsram->fields.mesi == cache_mesi_invalid)
		return false;

	if (data->id == packet->bus_original_sender && packet->bus_cmd == bus_flush) {
		uint16_t index = address.fields.index * BLOCK_SIZE + address.fields.offset;
		packet->bus_data = data->dsram[index];
	}

	Cache_mesi_e next_state = gSnoopingSM[tsram->fields.mesi](data, packet);

	if (address_offset == (BLOCK_SIZE - 1) || tsram->fields.mesi != cache_mesi_modified)
		tsram->fields.mesi = next_state;
	
	return true;
}

/**
 * @brief Handle cache response on the bus.
 * 
 * @param data Pointer to the cache data structure.
 * @param packet Pointer to the bus packet.
 * @param address_offset Pointer to the current address offset.
 * @return true if the response is complete, false otherwise.
 */
static bool cache_response_handle(CacheData_s* data, Bus_packet_s* packet, uint8_t* address_offset) {
	if (data->id != packet->bus_original_sender)
		return false;
		
	if (packet->bus_cmd != bus_flush)
		return false;
	
	cache_addess_s address = { .address = packet->bus_addr };
	Tsram_s* tsram = &(data->tsram[address.fields.index]);
	
	tsram->fields.tag = address.fields.tag;

	uint16_t index = address.fields.index * BLOCK_SIZE + address.fields.offset;
	data->dsram[index] = packet->bus_data;

	if (*address_offset == (BLOCK_SIZE - 1)) {
		tsram->fields.mesi = packet->bus_shared ? cache_mesi_shared : cache_mesi_exclusive;
		return true;
	}

	*address_offset += 1;
	return false;
}

/**
 * @brief State machine for snooping - invalid state.
 * 
 * @param data Pointer to the cache data structure.
 * @param packet Pointer to the bus packet.
 * @return Cache_mesi_e 
 */
static Cache_mesi_e mesi_snooping_invalid_state(CacheData_s* data, Bus_packet_s* packet)
{
	return cache_mesi_invalid;
}

/**
 * @brief State machine for snooping - shared state.
 * 
 * @param data Pointer to the cache data structure.
 * @param packet Pointer to the bus packet.
 * @return Cache_mesi_e 
 */
static Cache_mesi_e mesi_snooping_shared_state(CacheData_s* data, Bus_packet_s* packet) {
	if (packet->bus_cmd == bus_busRdX)
		return cache_mesi_invalid;

	return cache_mesi_shared;
}

/**
 * @brief State machine for snooping - exclusive state.
 * 
 * @param data Pointer to the cache data structure.
 * @param packet Pointer to the bus packet.
 * @return Cache_mesi_e 
 */
static Cache_mesi_e mesi_snooping_exclusive_state(CacheData_s* data, Bus_packet_s* packet) {
	if (packet->bus_cmd == bus_busRd)
		return cache_mesi_shared;

	if (packet->bus_cmd == bus_busRdX)
		return cache_mesi_invalid;

	return cache_mesi_exclusive;
}

/**
 * @brief State machine for snooping - modified state.
 * 
 * @param data Pointer to the cache data structure.
 * @param packet Pointer to the bus packet.
 * @return Cache_mesi_e 
 */
static Cache_mesi_e mesi_snooping_modified_state(CacheData_s* data, Bus_packet_s* packet) {
	cache_addess_s address = { .address = packet->bus_addr };
	uint16_t index = address.fields.index * BLOCK_SIZE + address.fields.offset;

	if (packet->bus_cmd == bus_busRd)
	{
		packet->bus_cmd = bus_flush;
		packet->bus_data = data->dsram[index];
		packet->bus_origid = data->id;

		return cache_mesi_shared;
	}

	else if (packet->bus_cmd == bus_busRdX) {
		packet->bus_cmd = bus_flush;
		packet->bus_data = data->dsram[index];
		packet->bus_origid = data->id;

		return cache_mesi_invalid;
	}

	else if (packet->bus_cmd == bus_flush) {
		packet->bus_cmd = bus_flush;
		packet->bus_data = data->dsram[index];
		packet->bus_origid = data->id;

		return cache_mesi_modified;
	}

	return cache_mesi_modified;
}
