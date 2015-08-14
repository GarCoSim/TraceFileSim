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
#include <stdlib.h>
#include <climits>
#include "../defines.hpp"
#include <string>

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
	void setAllocationSearchStart(int address);

	//stats
	void printMap();
	void printStats();
	virtual void freeAllSectors();
	virtual void freeOldSpace();

	void setHalfHeapSize(bool value);
	virtual void moveObject(Object *object);
	void swapHeaps();

	virtual bool isInNewSpace(Object *object);

	virtual void initializeHeap(int heapSize);

	virtual bool isRealAllocator();

protected:
	int getUsedSpace(bool newSpace);
	size_t allocateInNewSpace(int size);
	void setAllocated(int address, int size);
	void setFree(int address, int size);
	bool isBitSet(unsigned int address);
	void setBitUsed(unsigned int address);
	void setBitUnused(unsigned int address);
	virtual size_t allocate(int size, int lower, int upper, size_t lastAddress);


	bool isSplitHeap;
	char* myHeapBitMap;

	int myHeapSizeOldSpace;
	int myHeapSizeNewSpace;
	size_t myLastSuccessAddressOldSpace;
	size_t myLastSuccessAddressNewSpace;
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
