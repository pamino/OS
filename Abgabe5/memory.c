#include <errno.h>

#include "memory.h"
#include "bitset.h"

/**@brief Order of the largest possible memory block. */
#define ORDER_MAX 10

/**@brief Size of the smallest possible memory block. */
#define ORDER_PAGE 6

/**@brief Size of available memory. */
#define HEAP_SIZE (1 << ORDER_PAGE << ORDER_MAX)

#define TREE_SIZE (2*(1 << ORDER_MAX))/32


/**@brief Heap memory. */
static char heap[HEAP_SIZE];

static int freeMem_[ORDER_MAX][3];
static void* freeMemDyn_[ORDER_MAX];
static int bitsFreeMem_[ORDER_MAX][1];
static int tree_[TREE_SIZE];

int traverseRight(int num);
int traverseLeft(int num);
int bits2tree(int bits[], int level);

void mergeBuddies(int blockAdrL, int power);

int check(int arr[]);

void delete(void* dynArr, int pos);
int get(void* dynArr, int value);
void clear(int arr[], void* dynArr);
void push(void** dynArr, int x, int y);

void mem_init() {
	freeMem_[ORDER_MAX - 1][0] = 0;
	freeMem_[ORDER_MAX - 1][1] = HEAP_SIZE;

	// Has an Element(boolean value)
	freeMem_[ORDER_MAX - 1][2] = 1;

	//tracking if there is free memory
}

void* mem_alloc(size_t size) {
	if (size == 0)
		goto fail;

	int power = 0;
	while (size != 0) {
		size = size >> 1;
		++power;
	}
	if (power > ORDER_MAX+ORDER_PAGE)
		goto fail;
	if (power < ORDER_PAGE)
		power = ORDER_PAGE;
	int normalizedP = power-ORDER_PAGE;

	int start;

	int traverse[1] = {0};
	for (start = normalizedP; freeMem_[start][2] != 1; ++start) {
		if (start == ORDER_MAX)
			goto fail;
	}
	cpyBits(traverse, bitsFreeMem_[start], ORDER_MAX);
	int level = ORDER_MAX - start -1;
	printf("level: %i\n", level);

	int MemLow = freeMem_[start][0];
	int MemHigh = freeMem_[start][1];

	clear(freeMem_[start], freeMemDyn_[start]);
	for (int It = start; It != normalizedP; --It) {
		int split = (MemLow + MemHigh) >> 1;
		freeMem_[It - 1][0] = split;
		freeMem_[It - 1][1] = MemHigh;
		freeMem_[It - 1][2] = 1;

		//TODO COPY
		cpyBits(bitsFreeMem_[It - 1], bitsFreeMem_[It], ORDER_MAX);
		setBit(bitsFreeMem_[It - 1], level);
		++level;

		MemHigh = split;
	}
	printf("tree: %i\n", bits2tree(traverse, level));
	setBit(tree_, bits2tree(traverse, level));
	printf("debug\n");
	return &heap[MemLow];

fail:
	errno = ENOMEM;
	return NULL;
}

void* mem_realloc(void *oldptr, size_t new_size) {
	void* ret = mem_alloc(new_size);
	mem_free(oldptr);
	return ret;
}

void mem_free(void *ptr) {
	int level = 0;
	int traverse[1] = {0};
	int low = 0;
	int high = HEAP_SIZE;
	while(level < ORDER_MAX) {
		int split = (low + high)/2;

		printf("split: %i\n", split);
		if (1) {
			printf ("pointer: %p\n", ptr);
			printf ("vs Heap-pointer: %p\n", (void*)&heap[low]);
		}

		if (ptr == (void*)&heap[low]) {
			printf("traverse: %i\n", traverse[0]);
			for (int pos = bits2tree(traverse, level); pos < TREE_SIZE * 32; pos = traverseLeft(pos), ++level) {
				printf("pos:%i\n", pos);
				if (testBit(tree_, pos)) {
					clearBit(tree_, pos);
					int pseudoAdr = ptr - (void*)&heap[0];
					mergeBuddies(pseudoAdr, ORDER_MAX - level -1);
					return;
				}
			}
			goto fail;
		}
		else if (ptr > (void*)&heap[split]) {
			low = split;
			setBit(traverse, level);
		}
		else {
			high = split;
		}
		++level;
	}
fail:
	printf("Coulnd't free anything!\n");
}

void mem_dump(FILE *file) {
	/* TODO: print the current state of the allocator */
}

void mergeBuddies(int blockAdrL, int power) {
	//pseudo adresses
	int blockSize = (1 << ORDER_PAGE << power);
	int blockAdrH = blockAdrL + blockSize;

	int buddyAdrL;
	int buddyAdrH;

	if (!check(freeMem_[power]) ) 
		return;
	for (int i = power; i < ORDER_MAX-1; ++i) { 

		if ((get(freeMemDyn_[power], blockAdrL)) != -1 ) {
			buddyAdrL = blockAdrH;
			buddyAdrH = blockAdrH + blockSize;

			blockAdrL = blockAdrL;
			blockAdrH = buddyAdrH;
		}
		else if ((get(freeMemDyn_[power], blockAdrL - blockSize)) != -1 ) {
			buddyAdrL = blockAdrL - blockSize;
			buddyAdrH = blockAdrL;

			blockAdrL = buddyAdrL;
			blockAdrH = blockAdrH;
		}
		else 
			return;
		if (freeMem_[i][2] == 0) {
			freeMem_[i][0] = blockAdrL;
			freeMem_[i][1] = blockAdrH;
		}
		else {
			push(&freeMemDyn_[i], freeMem_[i][0], freeMem_[i][1]);
		}
		blockSize *=2;
	}

	return;
}

void delete(void* dynArr, int pos) {
	if (!dynArr)
		return;
	int* numElements = ((int*) dynArr) + 1;
	for (int i = pos+2; i+2 < *numElements; i+=2) {
		((int*)dynArr)[i+2] = ((int*)dynArr)[i];
		((int*)dynArr)[i+3] = ((int*)dynArr)[i+1];
	}
	*numElements-=2;
}

int get(void* dynArr, int value) {
	if (!dynArr)
		return -1;
	int* numElements = ((int*) dynArr) + 1;
	for (int i = 0; i < *numElements; i+=2) {
		if (((int*)dynArr)[i] == value) {
			delete(dynArr, i);
			return ((int*) dynArr)[i+1];
		}

	}
	return -1;
}

void clear(int arr[], void* dynArr) {
	int* numElements;
	if (dynArr) numElements = ((int*) dynArr) + 1;
	if (!dynArr || *numElements == 0) {
		arr[2] = 0;
		arr[0] = -1;
		arr[1] = -1;
		return;
	}
	arr[0] = ((int*) dynArr + 2)[*numElements-2];
	arr[1] = ((int*) dynArr + 2)[*numElements-1];
	arr[2] = 1;
	*numElements =- 2;
}

void push(void** dynArr, int x, int y) {
	if (!*dynArr) {
		*dynArr = mem_alloc(64);
		*(int*)*dynArr = 2;
		*((int*)*dynArr +1) = 64;
		((int*)*dynArr)[0] = x;
		((int*)*dynArr)[1] = y;
	}
	else {
		int dealloc = 0;
		int* heapSize = (int*) *dynArr;
		int* numElements = heapSize + 1;
		int allocSize = (*numElements*2+2)*sizeof(int);
		if (! (allocSize + 2*sizeof(int) < *heapSize)) {
			void* new = mem_alloc(2**heapSize);
			memcpy(new, *dynArr, *heapSize);
			((int*) new)[0] = 2**heapSize;
			dealloc = 1;
		}
		int* pos = *dynArr + (sizeof(int)**numElements * 2 + 2);
		pos[0] = x;
		pos[1] = y;
		*numElements +=2;
		if(dealloc) 
			mem_free(*dynArr);
	}
}

int check(int arr[]) {
	return arr[2];
}

int traverseRight(int num) {
	return 2*(num)+2;
}

int traverseLeft(int num) {
	return 2*(num)+1;
}

int bits2tree(int bits[], int level) {
	int ret = 0;
	for (int i = 0; i < level; ++i) {
		if (testBit(bits, i)) 
			ret = traverseRight(ret);
		else 
			ret = traverseLeft(ret);
	}
	return ret;
}
