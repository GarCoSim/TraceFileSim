/*
 * Allocator.cpp
 *
 *  Created on: 2013-09-03
 *      Author: GarCoSim
 */

#include "Allocator.hpp"

extern int gLineInTrace;

using namespace std;

namespace traceFileSimulator {

//keep setHalfHeapSize() for now, replace it eventually with setNumberOfRegionsHeap
void Allocator::setHalfHeapSize(bool value) {
	if (value) {
		oldSpaceStartHeapIndex = 0;
		oldSpaceEndHeapIndex = overallHeapSize / 2;
		newSpaceStartHeapIndex = oldSpaceEndHeapIndex;
		newSpaceEndHeapIndex = overallHeapSize;
		isSplitHeap = true;
	}
	else {
		oldSpaceStartHeapIndex = 0;
		oldSpaceEndHeapIndex = overallHeapSize;
		isSplitHeap = false;
	}
}

//added by Johannes
void Allocator::setNumberOfRegionsHeap(int value) {
	
	if (value == 2) {
		numberOfRegions = value;
		regionSize = overallHeapSize/value;
		oldSpaceStartHeapIndex = 0;
		oldSpaceEndHeapIndex = overallHeapSize / 2;
		newSpaceStartHeapIndex = oldSpaceEndHeapIndex;
		newSpaceEndHeapIndex = overallHeapSize;
		isSplitHeap = true;
	}
	else if (value == 1) {
		numberOfRegions = value;
		regionSize = overallHeapSize;
		oldSpaceStartHeapIndex = 0;
		oldSpaceEndHeapIndex = overallHeapSize;
		isSplitHeap = false;
	}
	else if (value == 0) {
		//determine numberOfRegions and regionSize based on overallHeapSize
		unsigned int i, currentRegionSize, currentNumberOfRegions;
		currentRegionSize = 0;
		currentNumberOfRegions = 0;
		bool stop;
		stop = false;
		
		for (i = REGIONEXPONENT; !stop; i++) {
			currentRegionSize = (int)pow((float)2,(float)i) * 1000; //KB to Byte
			currentNumberOfRegions = overallHeapSize/currentRegionSize;
			
			if (currentNumberOfRegions <= MAXREGIONS) 
				stop = true;
			if (i >= 24) //2^24 = 16.8GB, should be more than enough space per region
				stop = true;
		}
		
		numberOfRegions = currentNumberOfRegions;
		regionSize = currentRegionSize;
		maxNumberOfEdenRegions = (int)(floor(EDENREGIONS * numberOfRegions)/100);
		
		//initialize regions
		Region* balancedRegion;
		int currentAddress = 0; 

		for (i = 0; i < numberOfRegions; i++) {
			balancedRegion = new Region ((void*)currentAddress, regionSize);

			balancedGCRegions.push_back(balancedRegion);
			freeRegions.push_back(i);
			
			currentAddress = currentAddress + regionSize;
		}
	}
}

Allocator::Allocator() {
}

int Allocator::getFreeSize() {
	unsigned int i;
	int count = 0;
	for (i=0; i<overallHeapSize; i++)
		if (!isBitSet(i))
			count++;

	return count;
}

void *Allocator::gcAllocate(int size) {
	return allocate(size, oldSpaceStartHeapIndex, oldSpaceEndHeapIndex);
}

void *Allocator::gcAllocate(int size,int thread) {
	return allocate(size, oldSpaceStartHeapIndex, oldSpaceEndHeapIndex,thread);
}

void *Allocator::allocateInNewSpace(int size) {
	return allocate(size, newSpaceStartHeapIndex, newSpaceEndHeapIndex);
}

bool Allocator::isInNewSpace(Object *object) {
	return false;
}

void Allocator::swapHeaps() {
	unsigned int tempIndex;

	tempIndex = newSpaceStartHeapIndex;
	newSpaceStartHeapIndex = oldSpaceStartHeapIndex;
	oldSpaceStartHeapIndex = tempIndex;

	tempIndex = newSpaceEndHeapIndex;
	newSpaceEndHeapIndex = oldSpaceEndHeapIndex;
	oldSpaceEndHeapIndex = tempIndex;

	tempIndex = newSpaceRememberedHeapIndex;
	newSpaceRememberedHeapIndex = oldSpaceRememberedHeapIndex;
	oldSpaceRememberedHeapIndex = tempIndex;
}

void Allocator::freeOldSpace() {
	setFree(oldSpaceStartHeapIndex, oldSpaceEndHeapIndex-oldSpaceStartHeapIndex);
}

int Allocator::getUsedSpace(bool newSpace) {
	unsigned int i, usedSpace = 0;
	if (newSpace) {
		for (i = newSpaceStartHeapIndex; i < newSpaceEndHeapIndex; i++)
			if (isBitSet(i))
				usedSpace++;
	} else {
		for (i = oldSpaceStartHeapIndex; i < oldSpaceEndHeapIndex; i++)
			if (isBitSet(i))
				usedSpace++;
	}
	return (int) usedSpace;
}

void Allocator::moveObject(Object *object) {
	return;
}


void Allocator::initializeHeap(int heapSize) {
}

void Allocator::setAllocated(unsigned int heapIndex, int size) {
	int i;
	unsigned int toMark = heapIndex;

	for (i = 0; i < size; i++) {
		setBitUsed(toMark);
		toMark++;
	}
}

void Allocator::setFree(unsigned int heapIndex, int size) {
	int i;
	unsigned int toFree = heapIndex;

	for (i = 0; i < size; i++) {
		setBitUnused(toFree);
		toFree++;
	}
}

int Allocator::getHeapSize() {
	return overallHeapSize;
}

int Allocator::getRegionSize() {
	// this method can be generalized/overridden to support an arbitrary number of regions.
	return isSplitHeap ? overallHeapSize / 2 : overallHeapSize;
}

std::vector<Region*> Allocator::getRegions() {
	return balancedGCRegions;
}


void Allocator::printMap() {
	fprintf(heapMap, "%7d", gLineInTrace);

	unsigned int i;
	for (i = 0; i < overallHeapSize; i++) {
		if (isBitSet(i) == 1) {
			fprintf(heapMap, "X");
		} else {
			fprintf(heapMap, "_");
		}
	}

	fprintf(heapMap, "\n");
}

inline bool Allocator::isBitSet(unsigned int heapIndex) {
	int byteNR = heapIndex>>3;
	int bit = 7 - heapIndex % 8;
	return ((myHeapBitMap[byteNR] & (1 << bit))>0)?true:false;
}

void Allocator::setBitUsed(unsigned int heapIndex) {
	if (heapIndex > (unsigned int) overallHeapSize) {
		fprintf(stderr,
				"ERROR(Line %d): setBitUsed request to illegal slot %d\n",
				gLineInTrace, heapIndex);
		exit(1);
	}

	int byte = heapIndex / 8;
	int bit = 7 - heapIndex % 8;

	myHeapBitMap[byte] = myHeapBitMap[byte] | 1 << bit;
}

void Allocator::setBitUnused(unsigned int heapIndex) {
	if (heapIndex > (unsigned int) overallHeapSize) {
		fprintf(stderr, "add %d heap %zd\n", heapIndex, overallHeapSize);
		fprintf(stderr, "ERROR: setBitUnused request to illegal slot\n");
	}

	int byte = heapIndex / 8;
	int bit = 7 - heapIndex % 8;

	myHeapBitMap[byte] = myHeapBitMap[byte] & ~(1 << bit);
}

void Allocator::printStats() {
	if (DEBUG_MODE && WRITE_HEAPMAP) {
		printMap();
	}

	int bytesAllocated = overallHeapSize - getFreeSize();

	fprintf(allocLog, "%7d: alloc: %7d obj: %7d\n", gLineInTrace,
			bytesAllocated, statLiveObjects);
}

void Allocator::resetRememberedAllocationSearchPoint() {
	newSpaceRememberedHeapIndex = newSpaceStartHeapIndex;
	oldSpaceRememberedHeapIndex = oldSpaceStartHeapIndex;
}

bool Allocator::isRealAllocator() {
	return false;
}

void Allocator::freeAllSectors() {
}

void Allocator::gcFree(Object* object) {
}

void *Allocator::allocate(int size, int lower, int upper) {
	return NULL;
}

void *Allocator::allocate(int size, int lower, int upper, int thread) {
	return NULL;
}

void Allocator::printStats(long trigReason) {
}

Allocator::~Allocator() {
}

}
