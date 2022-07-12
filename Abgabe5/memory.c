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

typedef struct MemBlock {
	int low;
	int high;
	int set;
	Bitset bits;
} MemBlock;


/**@brief Heap memory. */
static char heap[HEAP_SIZE];

static MemBlock freeMem_[ORDER_MAX+1];
static void* freeMemDyn_[ORDER_MAX+1];
static unsigned tree_[TREE_SIZE];

int findEle(int pHandle, int* pos, Bitset* bits);
void mergeBuddies(MemBlock block, int power);
void clearTop(MemBlock* b, void* dynArr);
void setTop(MemBlock* b, int low, int high, Bitset bits);
int checkTop(MemBlock b);

void delete(void* dynArr, int pos);
int get(void* dynArr, int value);
void push(void** dynArr, MemBlock x);

int traverseRight(int num);
int traverseLeft(int num);
int bits2tree(Bitset bits, int level);

void printFreeMem() {
	int free = 0 ;
	for (int i = 0; i < ORDER_MAX + 1; ++i) {
		free += (freeMem_[i].high - freeMem_[i].low);
	}
	printf("free Memory: %i\n", free);
}

void mem_init() {
	freeMem_[ORDER_MAX] = (MemBlock){.low=0, .high=HEAP_SIZE, .set=1, .bits=0};
	for (int i =0; i < ORDER_MAX; ++i) {
		freeMem_[i] = (MemBlock){-1, -1, 0, 0};
	}
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

	Bitset traverse = 0;
	for (start = normalizedP; freeMem_[start].set != 1; ++start) {
		if (start > ORDER_MAX)
			goto fail;
	}
	cpyBits(&traverse, freeMem_[start].bits, ORDER_MAX);
	int level = ORDER_MAX - start;

	int MemLow = freeMem_[start].low;
	int MemHigh = freeMem_[start].high;
	clearTop(&freeMem_[start], freeMemDyn_[start]);
	for (int It = start; It != normalizedP; --It) {
		int split = (MemLow + MemHigh) >> 1;
		setTop(&freeMem_[It - 1], split, MemHigh, traverse);
		bitsetSet(&freeMem_[It - 1].bits, level);
		MemHigh = split;

		++level;
	}
	//printf("setting Bit: %i\n", bits2tree(traverse, level));
	int bit[1] = { traverse };
	setBit(tree_, bits2tree(traverse, level));
	//printf ("return pHandle: %i\n", MemLow);
	return &heap[MemLow];

fail:
	printf("couldn't allocate memory!\n");
	errno = ENOMEM;
	return NULL;
}

void* mem_realloc(void *oldptr, size_t new_size) {
	int pHandle = (char*)oldptr - &heap[0];

	void* ret = mem_alloc(new_size);
	//printBits(tree_, TREE_SIZE*32, 50);
	int pos;
	Bitset bits;
	int power = findEle(pHandle, &pos, &bits);
	if (power == -1) {
		printf("couldn't find element!");
		return NULL;
	}
	memcpy(ret, oldptr, 1 << ORDER_PAGE << power);
	mem_free(oldptr);
	return ret;
}

void mem_free(void *ptr) {
	int pHandle = (char*)ptr - &heap[0];
	//printf("freeing pHandle: %i\n", pHandle);
	int pos;
	int power;
	Bitset bits;
	if ((power = findEle(pHandle, &pos, &bits)) == -1)
	 	goto fail;
	clearBit(tree_, pos);
	mergeBuddies((MemBlock){pHandle,pHandle+(1 << ORDER_PAGE << power), 1, bits}, power);
	return;
fail:
	printf("Coulnd't free anything!\n");
}

void mem_dump(FILE *file) {
	/* TODO: print the current state of the allocator */
}

int findEle(int pHandle, int* pos, Bitset* bits) {
	Bitset traverse = 0;
	int level = 0;
	int low = 0;
	int high = HEAP_SIZE;
	while(level <= ORDER_MAX) {
		int split = (low + high)/2;
		if (pHandle == low) {
			for (*pos = bits2tree(traverse, level); *pos <= TREE_SIZE * 32; *pos = traverseLeft(*pos), ++level) {
				//printBits(tree_, 10, 10);
				//printf("testing Bit: %i\n", *pos);
				if (testBit(tree_, *pos)) {
					*bits = traverse;
					return ORDER_MAX - level;
				}
			}
		}
		else if (pHandle >= split) {
			low = split;
			bitsetSet(&traverse, level);
		}
		else {
			high = split;
		}
		++level;
	}
	return -1;
}

void mergeBuddies(MemBlock block, int power) {
	//pseudo adresses
	int blockSize = block.high-block.low;
	int blockAdrL = block.low;
	int blockAdrH = block.high;
	Bitset bits = block.bits;

	for (int i = power; i <= ORDER_MAX; ++i) { 

		if (!checkTop(freeMem_[i])) {
			setTop(&freeMem_[i], blockAdrL, blockAdrH, bits);
			return;
		}
		int budNum = blockAdrL / blockSize;
		int buddyAdress = budNum % 2 == 0 ? blockAdrL + blockSize : blockAdrL - blockSize;
		Bitset buddyBits = -1;
		if ((freeMem_[i].low == buddyAdress) ||
			(get(freeMemDyn_[i], buddyAdress, &buddyBits)) != -1 ) {
			if (buddyBits == -1)
				buddyBits = freeMem_[i].bits;
			blockAdrL = blockAdrL < buddyAdress? blockAdrL : buddyAdress;
			blockSize *= 2;
			blockAdrH = blockAdrL + blockSize;
			bits = blockAdrL < buddyAdress ? bits : buddyBits;
			clearTop(&freeMem_[i], freeMemDyn_[i]);
		}
		else {
			push(&freeMemDyn_[i], (MemBlock){blockAdrL, blockAdrH, 1, bits});
			return;
		}
	}

	return;
}

void delete(void* dynArr, int pos) {
	if (!dynArr)
		return;
	int size = 3;
	int* numElements = ((int*) dynArr) + 1;
	for (int i = pos; i+size < *numElements; i+=3) {
		((int*)dynArr)[i+size] = ((int*)dynArr)[i];
		((int*)dynArr)[i+size+1] = ((int*)dynArr)[i+1];
		((int*)dynArr)[i+size+2] = ((int*)dynArr)[i+2];
	}
	*numElements-=1;
}

int get(void* dynArr, int value, Bitset* bits) {
	if (!dynArr)
		return -1;
	int size = 3;
	//----debug
	int* arr = (int*)dynArr;
	int* numElements = arr + 1;
	for (int i = 0; i < *numElements; ++i) {
		int offset = i*size+2;
		if (arr[offset] == value) {
			delete(dynArr, offset);
			*bits = arr[offset + 2];
			return arr[offset+1];
		}

	}
	return -1;
}

void clearTop(MemBlock* b, void* dynArr) {
	int size = 3;
	int* numElements = NULL;

	if (dynArr) numElements = ((int*) dynArr) + 1;
	if (!dynArr || *numElements <= 0) {
		*b = (MemBlock) {-1,-1,0,0};
		return;
	}
	int offset = (*numElements*size+2);
	
	b->low = ((int*) dynArr)[offset-size];
	b->high = ((int*) dynArr)[offset-size+1];
	b->bits = ((int*) dynArr)[offset-size+2];
	b->set = 1;
	*numElements -= 1;
}

void setTop(MemBlock* b, int low, int high, Bitset bits) {
	if (b->set != 0) 
		printf("ERROR: overwritten Element");
	*b = (MemBlock) {low, high, 1, bits};
}

void push(void** dynArr, MemBlock x) {
	if (!*dynArr) {
		int allocSize = 1 << ORDER_PAGE > 5*sizeof(int) ? 1 << ORDER_PAGE : 5 * sizeof(int);
		*dynArr = mem_alloc(allocSize);
		int* arr = (int*)(*dynArr);
		arr[0] = allocSize;
		arr[1] = 1;
		arr[2] = x.low;
		arr[3] = x.high;
		arr[4] = x.bits;
	}
	else {
		int dealloc = 0;
		int* heapSize = (int*) *dynArr;
		int* numElements = heapSize + 1;
		int size = 3;
		int elements = *numElements*size+2;
		int allocSize = (elements+size)*sizeof(int);
		if (((allocSize + 3*sizeof(int)) > *heapSize)) {
			void* new = mem_alloc(2**heapSize);
			memcpy(new, *dynArr, *heapSize);
			((int*) new)[0] = 2**heapSize;
			dealloc = 1;
		}
		int* pos = (int*) *dynArr + elements;
		pos[0] = x.low;
		pos[1] = x.high;
		pos[2] = x.bits;
		*numElements +=1;
		if(dealloc) {
			mem_free(*dynArr);
		}
	}
}

int checkTop(MemBlock b) {
	return b.set;
}

int traverseRight(int num) {
	return (2*num)+2;
}

int traverseLeft(int num) {
	return (2*num)+1;
}

int bits2tree(Bitset bits, int level) {
	int ret = 0;
	for (int i = 0; i < level; ++i) {
		if (bitsetGet(&bits, i)) 
			ret = traverseRight(ret);
		else 
			ret = traverseLeft(ret);
	}
	return ret;
}
