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
#include "../Main/Region.hpp"
#include <string>

namespace traceFileSimulator {

class Allocator {
public:
	Allocator();
	virtual ~Allocator();

    void *gcAllocate(int size);
	void *gcAllocate(int size,int thread);
	virtual void gcFree(Object* object);

	//used mainly by garbage collector
	int getFreeSize();
	int getHeapSize();
	int getRegionSize();
	void resetRememberedAllocationSearchPoint();
	void setNumberOfRegionsHeap(int value);
	std::vector<Region*> getRegions();

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
	virtual void printStats(long trigReason);

protected:
	int getUsedSpace(bool newSpace);
	void *allocateInNewSpace(int size);
	void setAllocated(unsigned int heapIndex, int size);
	void setFree(unsigned int heapIndex, int size);
	bool isBitSet(unsigned int heapIndex);
	void setBitUsed(unsigned int heapIndex);
	void setBitUnused(unsigned int heapIndex);

    virtual void *allocate(int size, int lower, int upper);
	virtual void *allocate(int size, int lower, int upper,int thread);
   

	bool isSplitHeap;
	char* myHeapBitMap;

	std::vector<Region*> balancedGCRegions;
	unsigned int numberOfRegions;
	unsigned int regionSize;
	
	size_t overallHeapSize;
	unsigned int newSpaceStartHeapIndex;
	unsigned int oldSpaceStartHeapIndex;
	unsigned int oldSpaceEndHeapIndex;
	unsigned int newSpaceEndHeapIndex;
	unsigned int newSpaceRememberedHeapIndex;
	unsigned int oldSpaceRememberedHeapIndex;

	int statLiveObjects;
	FILE* allocLog;
	FILE* heapMap;
};

} 
#endif /* ALLOCATOR_HPP_ */
