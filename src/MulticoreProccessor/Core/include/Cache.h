#ifndef __CACHE_H__
#define __CACHE_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define CACHE_SIZE 512    
#define BLOCK_SIZE 8     
#define FRAME_SIZE (CACHE_SIZE / BLOCK_SIZE)  

typedef enum {
    cache_core0,
    cache_core1,
    cache_core2,
    cache_core3,
} Cache_Id_e;

typedef enum {
    cache_mesi_invalid,
    cache_mesi_shared,
    cache_mesi_exclusive,
    cache_mesi_modified,
    cache_mesi_max_state,
} Cache_mesi_e;

typedef union {
    uint32_t data;

    struct {
        uint16_t tag : 12; 
        uint16_t mesi : 2;  
    } fields;
} Tsram_s;

typedef struct {
    uint32_t read_hits;
    uint32_t write_hits;
    uint32_t read_misses;
    uint32_t write_misses;
} CacheStatistics_s;

typedef struct {
    Cache_Id_e id;
    bool memory_stall;
    uint32_t dsram[CACHE_SIZE];    
    Tsram_s tsram[FRAME_SIZE];       
    CacheStatistics_s statistics;
} CacheData_s;

void Cache_Init(CacheData_s* data, Cache_Id_e id);

void Cache_RegisterBusHandles(void);

bool Cache_ReadData(CacheData_s* cache_data, uint32_t address, uint32_t* data);

bool Cache_WriteData(CacheData_s* cache_data, uint32_t address, uint32_t data);

void Cache_PrintData(CacheData_s* cache_data, FILE* dsram_file, FILE* tsram_file);

void Cache_FlushToMemory(CacheData_s* cache_data);

#endif 
