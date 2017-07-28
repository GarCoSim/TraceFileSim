/*
 * Allocator.hpp
 *
 *  Created on: 2013-09-03
 *      Author: GarCoSim
 */

#ifndef _ALLOCATOR_HPP_
#define _ALLOCATOR_HPP_
#include <stdio.h>
#include "../Main/Region.hpp"
#include "../Main/Optional.cpp"

namespace traceFileSimulator {

class Object;

/** Abstract Allocator, requires specific implementation to use.
 * Allocators find places in the heap for objects to be place.
 */
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
	void setNumberOfRegionsHeap(size_t value);
	std::vector<Region*> getRegions();
	std::vector<unsigned int> getEdenRegions();
	std::vector<unsigned int> getFreeRegions();
	unsigned int getNextFreeRegionID();
	size_t getObjectRegion(Object* object);
	size_t getObjectRegionByRawObject(void* object);
	unsigned char *getHeap();
	void addNewFreeRegion(size_t regionID);
	void removeEdenRegion(size_t regionID);
	virtual size_t getSpaceToNextObject(size_t start);
	virtual unsigned char *getNextObjectAddress(size_t start);

	//stats
	void printMap();
	void printStats();
	virtual void freeOldSpace();
	void setRegionFree(Region* region);

	Optional<size_t>* getRegionIndex(Region* region);

	void setHalfHeapSize(bool value);
	virtual void moveObject(Object *object);
	void swapHeaps();

	virtual bool isInNewSpace(Object *object);

	virtual void initializeHeap(size_t heapSize);
	virtual void initializeHeap(size_t heapSize, size_t maxHeapSize);

	virtual bool isRealAllocator();
	virtual void printStats(long trigReason);

	virtual int addRegions();
	virtual int mergeRegions();

protected:
	size_t getUsedSpace(bool newSpace);
	void *allocateInNewSpace(size_t size);
	void setFree(size_t heapIndex, size_t size);
	bool isBitSet(size_t heapIndex);
	void setBitUsed(size_t heapIndex);
	void setBitUnused(size_t heapIndex);
	size_t getHeapIndex(Object *object);

	virtual void *allocate(size_t size, size_t lower, size_t upper);

	bool isSplitHeap;
	char* myHeapBitMap;

	std::vector<Region*> balancedRegions;
	std::vector<unsigned int> edenRegions;
	std::vector<unsigned int> freeRegions;

	size_t numberOfRegions;
	size_t regionSize;
	size_t maxNumberOfEdenRegions;
	size_t maximumMerges;

	size_t overallHeapSize;
	size_t maximumHeapSize;
	size_t newSpaceStartHeapIndex;
	size_t oldSpaceStartHeapIndex;
	size_t oldSpaceEndHeapIndex;
	size_t newSpaceEndHeapIndex;
	size_t newSpaceRememberedHeapIndex;
	size_t oldSpaceRememberedHeapIndex;

	typedef struct heapStats{
		unsigned char *heapStart;
		unsigned char *heapEnd;
		size_t firstRegion; //ID of first region in heap
	} heapStats;

	unsigned char *heap;
	std::vector<heapStats> allHeaps; //vector containing start and end pointers for all heaps (region-based)

	int statLiveObjects;
	FILE* allocLog;
	FILE* heapMap;
};

}
#endif /* ALLOCATOR_HPP_ */
