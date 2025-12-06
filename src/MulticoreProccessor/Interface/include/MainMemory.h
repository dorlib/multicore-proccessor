#ifndef MAINMEMORY_H
#define MAINMEMORY_H

#include <stdint.h>

#define MAIN_MEMORY_SIZE (0x200000)

void MainMemory_Init(void);

void MainMemory_PrintData(void);

void MainMemory_DirectWrite(uint32_t address, uint32_t data);

#endif 
