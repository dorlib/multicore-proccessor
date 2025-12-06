#ifndef FILES_H_
#define FILES_H_

#include <stdio.h>
#include "Helpers.h"

typedef struct {
	FILE* InstructionMemFile;
	FILE* RegFile;
	FILE* TraceFile;
	FILE* DsRamFile;
	FILE* TsRamFile;
	FILE* StatsFile;
} Core_Files;

extern Core_Files CoresFilesArray[NUMBER_OF_CORES];

FILE* MeminFile;
FILE* MemoutFile;
FILE* BusTraceFile;


int Files_OpenAll(char* argv[], int argc);

void CloseFiles(void);

#endif
