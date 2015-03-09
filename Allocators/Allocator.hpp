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

	virtual size_t gcAllocate(int size);
	virtual void gcFree(Object* object);

	//used mainly by garbage collector
	virtual int getFreeSize();
	virtual int getHeapSize();
	virtual void setAllocationSeearchStart(int address);

	//stats
	virtual void printMap();
	virtual void printStats();
	virtual void freeAllSectors();

	void setHalfHeapSize(bool value);
	virtual void moveObject(Object *object);
	virtual void swapHeaps();

	virtual bool isInNewSpace(Object *object);

	virtual void initializeHeap(int heapSize);

	virtual bool isRealAllocator();

protected:
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
