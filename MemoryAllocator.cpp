#include <stdlib.h>
#include <stdint.h>
#include <cassert>
#include <string>
#include <iostream>
using namespace std;

#define HEAP_SIZE 128
#define BLOCK_AMOUNT 60
#define HEADER 8

#define IS_OCCUPIED(i) *(uint8_t*)(i - 4)
#define PREV_SIZE(i) *(uint8_t*)(i - 6)
#define SIZE(i) *(uint8_t*)(i - 8)

#define GET_IS_OCCUPIED(i) (uint8_t*)(i - 4)
#define GET_PREV_SIZE(i) (uint8_t*)(i - 6)
#define GET_SIZE(i) (uint8_t*)(i - 8)

class MemoryAllocator
{
	static char heap[HEAP_SIZE];
	static uint8_t* heapHead;
	static uint16_t totalBlocks;

	void InsertHeader(uint8_t* address, uint8_t currentSize, uint8_t previousSize, bool isOccupied);


public:
	MemoryAllocator();

	void* mem_alloc(size_t size);
	void* mem_realloc(void* addr, size_t size);
	void mem_free(void* addr);
	void mem_show();
};

char MemoryAllocator::heap[HEAP_SIZE];
uint8_t* MemoryAllocator::heapHead;
uint16_t MemoryAllocator::totalBlocks;

void MemoryAllocator::InsertHeader(uint8_t* address, uint8_t currentSize, uint8_t previousSize, bool isOccupied)
{
	*address = currentSize; // 2
	*(address + 2) = previousSize; // 4
	*(address + 4) = isOccupied; // 5
}
MemoryAllocator::MemoryAllocator()
{
	cout << "The heap is created..." << endl;
	totalBlocks = 0;

	InsertHeader((uint8_t*)heap, 120, 0, false);
	++totalBlocks;

	heapHead = (uint8_t*)(heap + HEADER);
}
void* MemoryAllocator::mem_alloc(size_t size)
{
	cout << "Allocating " << size << " bytes + " << HEADER << " bytes HEADER = " << HEADER + size << " bytes..." << endl;

	uint8_t* tempPtr = (uint8_t*)(heap + HEADER);

	uint8_t sizeNeeded = size + HEADER;

	uint8_t* resultAddress = nullptr;

	for (size_t i = 1; i <= totalBlocks; i++)
	{
		if (!IS_OCCUPIED(tempPtr) && SIZE(tempPtr) >= sizeNeeded)
		{
			int tempSize = SIZE(tempPtr);

			InsertHeader(tempPtr + size, tempSize - sizeNeeded, size, 0);

			IS_OCCUPIED(tempPtr) = 1;
			SIZE(tempPtr) = size;

			++totalBlocks;
			resultAddress = tempPtr;
		}
		else if (i < totalBlocks)
		{
			tempPtr += (SIZE(tempPtr) + HEADER);
		}
	}

	return resultAddress;
}
void* MemoryAllocator::mem_realloc(void* addr, size_t size)
{
	cout << "Reallocating value " << *(int*)addr << " on address " << addr << ": " << (int)SIZE((uint8_t*)addr) << " bytes -> " << size << " bytes..." << endl;

	if (addr == NULL) return mem_alloc(size);

	uint8_t* tempCurrPtr = (uint8_t*)addr;

	if (size + HEADER <= SIZE(tempCurrPtr))
	{
		InsertHeader(tempCurrPtr + size, SIZE(tempCurrPtr) - size - HEADER, size, false);

		SIZE(tempCurrPtr) = size;
		IS_OCCUPIED(tempCurrPtr) = true;
		totalBlocks++;
	}
	else
	{
		uint8_t* tempPtr = (uint8_t*)(heap + HEADER);
		uint8_t sizeNeeded = size + HEADER;
		uint8_t* resultAddress = nullptr;

		int currentBlock = totalBlocks;

		bool PrevFree = PREV_SIZE(tempCurrPtr) >= size - SIZE(tempCurrPtr) + HEADER ? !IS_OCCUPIED(tempCurrPtr - HEADER - PREV_SIZE(tempCurrPtr)) : false;

		bool NextFree = !IS_OCCUPIED(tempCurrPtr + SIZE(tempCurrPtr) + HEADER);

		if (NextFree)
		{
			uint8_t* tempNextPtr = tempCurrPtr + HEADER + SIZE(tempCurrPtr);


			int tempNextSize = SIZE(tempNextPtr) - size + SIZE(tempCurrPtr);

			InsertHeader(tempCurrPtr + size, tempNextSize, size, false);

			SIZE(tempCurrPtr) = size;
			IS_OCCUPIED(tempCurrPtr) = true;
			resultAddress = tempCurrPtr;
		}
		else if (PrevFree)
		{
			// found the previous block pointer
			uint8_t* tempPrevPtr = tempCurrPtr - HEADER - PREV_SIZE(tempCurrPtr);

			// copying the data to the new location
			for (size_t i = 0; i < SIZE(tempCurrPtr); i++)
			{
				*(tempPrevPtr + i) = *(tempCurrPtr + i);
			}
			
			InsertHeader(tempPrevPtr + size, SIZE(tempCurrPtr) + SIZE(tempPrevPtr) - size, size, false);

			SIZE(tempPrevPtr) = size;
			IS_OCCUPIED(tempPrevPtr) = true;

			mem_free(tempPrevPtr + size + HEADER);
			
			resultAddress = tempPrevPtr;
		}
		else
		{
			tempPtr = (uint8_t*)mem_alloc(size);

			if (tempPtr != NULL)
			{
				for (size_t i = 0; i < SIZE(tempCurrPtr); i++)
				{
					*(tempPtr + i) = *(tempCurrPtr + i);
				}

				mem_free(tempCurrPtr);
				resultAddress = tempPtr;
			}
			else
			{
				cout << "Couldn't allocate enough memory!" << endl;
				return resultAddress;
			}
		}

		addr = nullptr;
		return resultAddress;
	}
}
void MemoryAllocator::mem_free(void* addr)
{
	cout << "Deleting " << *(int*)addr << " value -> " << (int)SIZE((uint8_t*)addr) << " bytes..." << endl;
	
	if (totalBlocks == 1)
	{
		cout << "Error deleting - only one block is present" << endl;
		return;
	}

	uint8_t* tempCurrPtr = (uint8_t*)(addr);
	bool PrevFree = PREV_SIZE(tempCurrPtr) > 0 ? !IS_OCCUPIED(tempCurrPtr - HEADER - PREV_SIZE(tempCurrPtr)) : false;

	uint8_t* tempPtr = (uint8_t*)(heap + HEADER);
	bool NextFree = !IS_OCCUPIED(tempCurrPtr + SIZE(tempCurrPtr) + HEADER);

	IS_OCCUPIED(tempCurrPtr) = false;
	*tempCurrPtr = NULL;

	uint8_t* tempPrevPtr = PrevFree ? (uint8_t*)(tempCurrPtr - HEADER - PREV_SIZE(tempCurrPtr)) : tempCurrPtr;
	uint8_t* tempNextPtr = NextFree ? (uint8_t*)(tempCurrPtr + HEADER + SIZE(tempCurrPtr)) : tempCurrPtr;

	// setting the correct size to the prev block
	if (PrevFree || NextFree)
	{
		SIZE(tempPrevPtr) += (HEADER * PrevFree) + (SIZE(tempCurrPtr) * PrevFree) +
			(HEADER * NextFree) + (SIZE(tempNextPtr) * NextFree);

		if (PrevFree) --totalBlocks;
		if (NextFree) --totalBlocks;
	}

	addr = nullptr;
}
void MemoryAllocator::mem_show()
{
	uint8_t* currPtr = (uint8_t*)(heap + HEADER);

	cout << "\n------------------------------ Memory Show ------------------------------" << endl;

	cout << "Currently the amount of blocks is: " << totalBlocks << endl;
	for (size_t i = 1; i <= totalBlocks; i++)
	{
		int tempSize = SIZE(currPtr);
		bool tempFree = !IS_OCCUPIED(currPtr);

		cout << "The block #" << i;
		cout << " -> Data: " << *(uint16_t*)currPtr;
		cout << ". Address: " << (uint16_t*)currPtr;
		cout << ". Size: " << tempSize;
		cout << ". Free: " << tempFree << endl;

		currPtr += (tempSize + HEADER);
	}
	cout << "-------------------------------------------------------------------------\n" << endl;
}

void Test1()
{
	MemoryAllocator allocator;

	allocator.mem_show();

	int* test1;
	test1 = (int*)allocator.mem_alloc(20);
	*test1 = 20;

	int* test2;
	test2 = (int*)allocator.mem_alloc(20);
	*test2 = 6;

	int* test3;
	test3 = (int*)allocator.mem_alloc(20);
	*test3 = 10;

	int* test4;
	test4 = (int*)allocator.mem_alloc(20);
	*test4 = 15;

	allocator.mem_show();

	allocator.mem_free(test3);

	allocator.mem_show();

	test2 = (int*)allocator.mem_realloc(test2, 25);

	allocator.mem_show();
}

int main()
{
	Test1();
	return 0;
}
