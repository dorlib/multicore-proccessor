#include "../include/Bus.h"
#include "../include/Helpers.h"
#include "../include/Files.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


typedef union {
	uint32_t address;

	struct {
		uint16_t offset : 3;
		uint32_t block : 18;
	} fields;
} memory_address_s;


typedef enum {
	transaction_idle_state,
	transaction_wait_state,
	transaction_operation_state,
	transaction_finally_state,
} transaction_state_e;


typedef struct _queue_item_s {
	Bus_packet_s item;
	struct _queue_item_s* parent;
	struct _queue_item_s* next;
} queue_item_s;


static Bus_cache_interface_s gCacheInterface[NUMBER_OF_CORES];

static shared_signal_callback	gSharedSignalCallback;
static cache_snooping_callback	gCacheSnoopingCallback;
static cache_response_callback	gCacheResponseCallback;
static memory_callback_t		gMemoryCallback;

static bool gBusInProgress;
static transaction_state_e gBusTransactionState[NUMBER_OF_CORES] = { 0, 0, 0, 0 };
static Bus_packet_s gCurrentPacket;
static uint8_t gAddressOffset;
static uint32_t gIterCounter = 0;

static queue_item_s* gQueueHead;
static queue_item_s* gQueueTail;

static bool check_shared_line(Bus_packet_s* packet, bool* is_modified);
static bool check_cache_snooping(Bus_packet_s* packet);
static void print_bus_status(Bus_packet_s packet);

bool bus_fifo_IsEmpty(void);
bool bus_fifo_Enqueue(Bus_packet_s item);
bool bus_fifo_Dequeue(Bus_packet_s* item);

/**	
 * @brief Register cache interface to the bus.
 * @param Bus_cache_interface_s cache_interface - cache interface structure.
 */
void Bus_RegisterCache(Bus_cache_interface_s cache_interface) {
	gCacheInterface[cache_interface.core_id] = cache_interface;
}

/**	
 * @brief Register cache callbacks to the bus.
 * @param shared_signal_callback signal_callback - shared signal callback function.
 * @param cache_snooping_callback snooping_callback - cache snooping callback function.
 * @param cache_response_callback response_callback - cache response callback function.
 */
void Bus_RegisterCacheCallbacks(shared_signal_callback signal_callback,
								cache_snooping_callback snooping_callback,
								cache_response_callback response_callback) {
	gSharedSignalCallback = signal_callback;
	gCacheSnoopingCallback = snooping_callback;
	gCacheResponseCallback = response_callback;
}

/**	
 * @brief Register memory callback to the bus.
 * @param memory_callback_t callback - memory callback function.
 */
void Bus_RegisterMemoryCallback(memory_callback_t callback){
	gMemoryCallback = callback;
}

/**	
 * @brief Create a new transaction on the bus.
 * @details this function add the transaction into the round-robin queue.
 * @param Bus_packet_s packet - bus packet structure.
 */
void Bus_AddTransaction(Bus_packet_s packet) {
	bus_fifo_Enqueue(packet);

	if (packet.bus_origid == bus_invalid_originator)
		return;

	gBusTransactionState[packet.bus_origid] = transaction_wait_state;
}

/**	
 * @brief Check if the bus is in transaction
 * @param Bus_originator_e originator - the id of the core.
 * @return true if the bus is in transaction
 */
bool Bus_InTransaction(Bus_originator_e originator){
	return gBusTransactionState[originator] != transaction_idle_state;
}

/**	
 * @brief Check if the bus is waiting for transaction
 * @param Bus_originator_e originator - the id of the core.
 * @return true if the bus is waiting
 */
bool Bus_WaitForTransaction(Bus_originator_e originator) {
	return gBusTransactionState[originator] == transaction_wait_state;
}

/**	
 * @brief Bus iteration
 */
void Bus_Iter(void) {
	static bool is_first_shared = true;
	gIterCounter++;
	
	if (gCurrentPacket.bus_origid < NUMBER_OF_CORES && 
	    gBusTransactionState[gCurrentPacket.bus_origid] == transaction_finally_state)
		gBusTransactionState[gCurrentPacket.bus_origid] = transaction_idle_state;

	if (bus_fifo_IsEmpty() && !gBusInProgress) {
		gCurrentPacket.bus_origid = bus_invalid_originator;
		return;
	}

	if (!gBusInProgress) {
		is_first_shared = true;
		int prev_origid = gCurrentPacket.bus_origid;
		
		if (!bus_fifo_Dequeue(&gCurrentPacket) || gCurrentPacket.bus_origid == bus_invalid_originator)
			return;

		gCurrentPacket.bus_original_sender = gCurrentPacket.bus_origid;

		gBusInProgress = true;
		gBusTransactionState[gCurrentPacket.bus_origid] = transaction_operation_state;
		gAddressOffset = 0;
		printf("bus trace - (#%d) dequeue next cmd\n", gIterCounter);
		print_bus_status(gCurrentPacket);
	}

	Bus_packet_s packet;
	memcpy(&packet, &gCurrentPacket, sizeof(gCurrentPacket));

	memory_address_s address = { .address = gCurrentPacket.bus_addr };
	address.fields.offset = gAddressOffset;
	packet.bus_addr = address.address;

	bool is_modified = false;
	packet.bus_shared = check_shared_line(&gCurrentPacket, &is_modified);
	if (is_modified && is_first_shared) {
		is_first_shared = false;
		return;
	}

	bool cache_response  = check_cache_snooping(&packet);
	bool memory_response = gMemoryCallback(&packet, is_modified);
	
	if (memory_response) {
		printf("bus trace - (#%d) response to sender\n", gIterCounter);
		print_bus_status(packet);

		if (gCacheResponseCallback(gCacheInterface[gCurrentPacket.bus_origid].cache_data, &packet, &gAddressOffset)) {
			gBusTransactionState[gCurrentPacket.bus_origid] = transaction_finally_state;
			gBusInProgress = false;
		}
	}
}

/**
 * @brief Check shared line on the bus.
 * 
 * @param packet Pointer to the bus packet.
 * @param is_modified Pointer to the is_modified flag.
 * @return true if shared line is set.
 */
static bool check_shared_line(Bus_packet_s* packet, bool* is_modified) {
	bool shared = false;

	for (int i = 0; i < NUMBER_OF_CORES; i++)
		shared |= gSharedSignalCallback(gCacheInterface[i].cache_data, packet, is_modified);
	
	return shared;
}

/**
 * @brief Check cache snooping on the bus.
 * 
 * @param packet Pointer to the bus packet.
 * @return true if any cache responded.
 */
static bool check_cache_snooping(Bus_packet_s* packet) {
	bool cache_response = false;
	for (int i = 0; i < NUMBER_OF_CORES; i++)
		cache_response |= gCacheSnoopingCallback(gCacheInterface[i].cache_data, packet, gAddressOffset);
	
	return cache_response;
}

/**
 * @brief Print the bus status to the trace file.
 * @param packet Bus packet to print.
 */
static void print_bus_status(Bus_packet_s packet) {
	fprintf(BusTraceFile, "%d %d %d %06X %08X %d\n", gIterCounter, packet.bus_origid, packet.bus_cmd, 
		packet.bus_addr, packet.bus_data, packet.bus_shared);
}

/**
 * @brief Check if the bus FIFO is empty.
 * 
 * @return true if the bus FIFO is empty.
 */
bool bus_fifo_IsEmpty(void) {
	return gQueueHead == NULL;
}

/**
 * @brief Enqueue a bus packet to the bus FIFO.
 * 
 * @param item Bus packet to enqueue.
 * @return true if the enqueue was successful.
 */
bool bus_fifo_Enqueue(Bus_packet_s item) {
	queue_item_s* queue_item = malloc(sizeof(queue_item_s));
	if (queue_item == NULL)
		return false;

	queue_item->item = item;
	queue_item->next = NULL;
	queue_item->parent = NULL;

	if (bus_fifo_IsEmpty()) {
		gQueueHead = queue_item;
		gQueueTail = queue_item;
	} else {
		gQueueHead->parent = queue_item;
		queue_item->next = gQueueHead;
		gQueueHead = queue_item;
	}
	return true;
}

/**
 * @brief Dequeue a bus packet from the bus FIFO.
 * 
 * @param item Pointer to store the dequeued bus packet.
 * @return true if the dequeue was successful.
 */
bool bus_fifo_Dequeue(Bus_packet_s* item) {
	if (gQueueTail == NULL)
		return false;

	queue_item_s* queue_item = gQueueTail;
	gQueueTail = queue_item->parent;

	if (gQueueTail == NULL)
		gQueueHead = NULL;
	else
		gQueueTail->next = NULL;

	queue_item->parent = NULL;
	*item = queue_item->item;

	free(queue_item);
	return true;
}
