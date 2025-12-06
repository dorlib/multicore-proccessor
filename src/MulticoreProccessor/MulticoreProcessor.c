#include "Interface/include/Files.h"
#include "Core/include/Core.h"
#include "Interface/include/MainMemory.h"
#include "Interface/include/Bus.h"
#include <string.h>


static Core_s cores[NUMBER_OF_CORES];

static bool ProcessorHalted();
static void AssignFiles(Core_s* cores);
static void CoresInit();


int main(int argc, char *argv[]) {

	if (Files_OpenAll(argv, argc)) {
		printf("Error opening files.\n");
		return 1;
	}

	MainMemory_Init();
	CoresInit();

	while (!ProcessorHalted()) {
		Bus_Iter();
		for (int core = 0; core < NUMBER_OF_CORES; core++) {
			Core_Iter(&cores[core]);
		}
	}

	for (int core = 0; core < NUMBER_OF_CORES; core++) {
		Core_Teaddown(&cores[core]);
	}

	MainMemory_PrintData();
	CloseFiles();
	return 0;
} 


/**
 * @brief Check if all cores are halted.
 * 
 * @return true if all cores are halted and false otherwise.
 */
static bool ProcessorHalted() {
	bool is_halted = true;
	for (int core = 0; core < NUMBER_OF_CORES; core++) {
		is_halted &= Core_isHalted(&cores[core]);
	}

	return is_halted;
}


/**
* @brief Assign files to cores.
* @param Core_s* cores - pointer to the cores array.
*/
static void AssignFiles(Core_s* cores) {
	for (int core = 0; core < NUMBER_OF_CORES; core++) {
		cores[core].core_files = CoresFilesArray[core];
	}
}


/**
 * @brief Initialize all cores.
 * 
 */
void CoresInit() {
	memset(cores, 0, NUMBER_OF_CORES * sizeof(Core_s));
	AssignFiles(cores);
	for (int i = 0; i < NUMBER_OF_CORES; i++) {
		Core_Init(&cores[i], i);
	}
}
