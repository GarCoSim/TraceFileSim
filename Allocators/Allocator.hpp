/*
 * Allocator.hpp
 *
 *  Created on: 2013-09-03
 *      Author: GarCoSim
 */

#ifndef _ALLOCATOR_HPP_
#define _ALLOCATOR_HPP_

#include "../Main/Object.hpp"
#include <stdio.h>

namespace traceFileSimulator {

class Allocator {
public:
	Allocator();
	virtual ~Allocator();

	size_t gcAllocate(int size);
	virtual void gcFree(Object* object);

	//used mainly by garbage collector
	int getFreeSize();
	int getHeapSize();
	void setAllocationSeearchStart(int address);

	//stats
	void printMap();
	void printStats();
	virtual void freeAllSectors();

	void setHalfHeapSize(bool value);
	void moveObject(Object *object);
	void swapHeaps();

	bool isInNewSpace(Object *object);

	void initializeHeap(int heapSize);

	virtual bool isRealAllocator();

protected:
	int getUsedSpace(bool newSpace);
	size_t allocateInNewSpace(int size);
	void setAllocated(int address, int size);
	void setFree(int address, int size);
	bool isBitSet(unsigned int address);
	void setBitUsed(unsigned int address);
	void setBitUnused(unsigned int address);
	virtual size_t allocate(int size, int lower, int upper, int lastAddress);


	bool isSplitHeap;
	char* myHeapBitMap;

	int myHeapSizeOldSpace;
	int myHeapSizeNewSpace;
	int myLastSuccessAddressOldSpace;
	int myLastSuccessAddressNewSpace;
	int newSpaceOffset;
	int oldSpaceOffset;
	int overallHeapSize;

	int statBytesAllocated;
	int statLiveObjects;
	FILE* allocLog;
	FILE* heapMap;
};

} 
#endif /* ALLOCATOR_HPP_ */
