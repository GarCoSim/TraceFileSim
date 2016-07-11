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

	void setAllocated(size_t heapIndex, size_t size);
	void setAllocated(unsigned char *heapStart, size_t heapIndex, size_t size);
	
	//used mainly by garbage collector
	size_t getFreeSize();
	size_t getHeapSize();
	size_t getRegionSize();
	void resetRememberedAllocationSearchPoint();
	void setNumberOfRegionsHeap(int value);
	std::vector<Region*> getRegions();
	std::vector<unsigned int> getEdenRegions();
	std::vector<unsigned int> getFreeRegions();
	unsigned int getNextFreeRegionID();
	unsigned int getObjectRegion(Object* object);
	unsigned int getObjectRegionByRawObject(void* object);
	unsigned char *getHeap();
	size_t getSpaceToNextObject(size_t start);
	unsigned char *getNextObjectAddress(size_t start);
	size_t getOldSpaceStartHeapIndex();
	size_t getOldSpaceEndHeapIndex();
	void addNewFreeRegion(unsigned int regionID);
	void removeEdenRegion(unsigned int regionID);

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
	virtual void initializeHeap(size_t heapSize, size_t maxHeapSize);

	virtual bool isRealAllocator();
	virtual void printStats(long trigReason);

	virtual int addRegions();

protected:
	size_t getUsedSpace(bool newSpace);
	void *allocateInNewSpace(size_t size);
	void setFree(size_t heapIndex, size_t size);
	bool isBitSet(size_t heapIndex);
	void setBitUsed(size_t heapIndex);
	void setBitUnused(size_t heapIndex);
	size_t getHeapIndex(Object *object);

    virtual void *allocate(size_t size, size_t lower, size_t upper) = 0;


	bool isSplitHeap;
	char* myHeapBitMap;

	std::vector<Region*> balancedRegions;
	std::vector<unsigned int> edenRegions;
	std::vector<unsigned int> freeRegions;

	unsigned int numberOfRegions;
	size_t regionSize;
	unsigned int maxNumberOfEdenRegions;

	size_t overallHeapSize;
	size_t maximumHeapSize;
	size_t newSpaceStartHeapIndex;
	size_t oldSpaceStartHeapIndex;
	size_t oldSpaceEndHeapIndex;
	size_t newSpaceEndHeapIndex;
	size_t newSpaceRememberedHeapIndex;
	size_t oldSpaceRememberedHeapIndex;

	unsigned char *heap;
	std::vector<unsigned char*> allHeaps; //vector containing start and end pointers for all heaps (region-based)

	int statLiveObjects;
	FILE* allocLog;
	FILE* heapMap;
};

}
#endif /* ALLOCATOR_HPP_ */
