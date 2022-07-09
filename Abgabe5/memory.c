#include <errno.h>

#include "memory.h"
#include "bitset.h"

/**@brief Order of the largest possible memory block. */
#define ORDER_MAX 10

/**@brief Size of the smallest possible memory block. */
#define ORDER_PAGE 6

/**@brief Size of available memory. */
#define HEAP_SIZE (2 << ORDER_PAGE << ORDER_MAX)


/**@brief Heap memory. */
static char heap[HEAP_SIZE];

static int[ORDER_MAX][3] freeMem_;

void mem_init() {
	freeMem[ORDER_MAX - 1][0] = 0;
	freeMem[ORDER_MAX - 1][1] = HEAP_SIZE;

	//tracking if there is free memory
	freeMem[ORDER_MAX - 1][2] = 1;
}

void* mem_alloc(size_t size) {
	if (size == 0)
		goto fail;

	int power = 0;
	while (size != 0) {
		size >> 1;
		++power;
	}
	if (power > ORDER_MAX + ORDER_PAGE)
		goto fail;
	if (power < ORDER_PAGE)
		power = ORDER_PAGE;
	int normalizedP = power - ORDER_PAGE;

	int start;
	for (start = normalizedP; freeMem[start][2] != 1; ++start) {
		if (start == ORDER_MAX)
			goto fail;
	}
	int MemLow = freeMem[start][0];
	int MemHigh = freeMem[start][1];

	freeMem[start][2] = 0;
	for (int It = start; It != normalizedP; --It) {
		int split = (MemLow + MemHigh) >> 1;
		freeMem[It - 1][0] = split;
		freeMem[It - 1][1] = MemHigh;
		freeMem[It - 1][2] = 1;

		MemHigh = split;
	}

	return &heap[MemLow];

fail:
	errno = ENOMEM;
	return NULL;
}

void* mem_realloc(void *oldptr, size_t new_size) {
	/* TODO: increase the size of an existing block through merging
	 *   OR  allocate a new one with identical contents */
fail:
	errno = ENOMEM;
	return NULL;
}

void mem_free(void *ptr) {
	/* TODO: mark a block as unused */
}

void mem_dump(FILE *file) {
	/* TODO: print the current state of the allocator */
}

void traverseRight(int num) {

}
