#include <stdio.h>
#include "memory.h"

int main() {
	int* array[4];
	mem_init();

	//Testfall 1 Heap space increases
	for(int j = 0; j < 500; ++j) {
		array[0] = mem_alloc(10*  sizeof(int));
		array[1] = mem_alloc(10 * sizeof(int));
		array[2] = mem_alloc(10 * sizeof(int));
		//array[3] = mem_alloc(10 * sizeof(int));
		array[1] = mem_realloc(array[1], 20 * sizeof(int));
		array[2] = mem_realloc(array[2], 20 * sizeof(int));
		array[0] = mem_realloc(array[0], 30 * sizeof(int));
		//array[3] = mem_realloc(array[3], 20 * sizeof(int));
		mem_free(array[1]);
		mem_free(array[2]);
		mem_free(array[0]);
		//mem_free(array[0]);
		printFreeMem();
	}
	
	
}
