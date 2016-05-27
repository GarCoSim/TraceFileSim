/*
 * Allocator.hpp
 *
 *  Created on: 2013-09-03
 *      Author: GarCoSim
 */

#ifndef _ALLOCATOR_HPP_
#define _ALLOCATOR_HPP_
#include <stdio.h>

namespace traceFileSimulator {

class Object;

class Allocator {
public:
	Allocator();
	virtual ~Allocator();

    void *gcAllocate(size_t size);
	virtual void gcFree(Object* object);

	//used mainly by garbage collector
	size_t getFreeSize();
	size_t getHeapSize();
	int getRegionSize();
	void resetRememberedAllocationSearchPoint();
	unsigned char *getHeap();

	//stats
	void printMap();
	void printStats();
	virtual void freeAllSectors();
	virtual void freeOldSpace();

	void setHalfHeapSize(bool value);
	virtual void moveObject(Object *object);
	void swapHeaps();

	virtual bool isInNewSpace(Object *object);

	virtual void initializeHeap(size_t heapSize);

	virtual bool isRealAllocator();
	virtual void printStats(long trigReason);

protected:
	size_t getUsedSpace(bool newSpace);
	void *allocateInNewSpace(size_t size);
	void setAllocated(size_t heapIndex, size_t size);
	void setFree(size_t heapIndex, size_t size);
	bool isBitSet(size_t heapIndex);
	void setBitUsed(size_t heapIndex);
	void setBitUnused(size_t heapIndex);
	size_t getHeapIndex(Object *object);

    virtual void *allocate(size_t size, size_t lower, size_t upper);
   

	bool isSplitHeap;
	char* myHeapBitMap;

	
	size_t overallHeapSize;
	size_t newSpaceStartHeapIndex;
	size_t oldSpaceStartHeapIndex;
	size_t oldSpaceEndHeapIndex;
	size_t newSpaceEndHeapIndex;
	size_t newSpaceRememberedHeapIndex;
	size_t oldSpaceRememberedHeapIndex;
	
	unsigned char *heap;

	int statLiveObjects;
	FILE* allocLog;
	FILE* heapMap;
};

} 
#endif /* ALLOCATOR_HPP_ */
