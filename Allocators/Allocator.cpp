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


Allocator::Allocator() {
}

size_t Allocator::getFreeSize() {
	size_t i;
	size_t count = 0;
	for (i=0; i<overallHeapSize; i++)
		if (!isBitSet(i))
			count++;

	return count;
}

unsigned char *Allocator::getHeap () {
	return heap;
}

void *Allocator::gcAllocate(size_t size) {
	return allocate(size, oldSpaceStartHeapIndex, oldSpaceEndHeapIndex);
}

void *Allocator::allocateInNewSpace(size_t size) {
	return allocate(size, newSpaceStartHeapIndex, newSpaceEndHeapIndex);
}

bool Allocator::isInNewSpace(Object *object) {
	return false;
}

void Allocator::swapHeaps() {
	size_t tempIndex;

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

size_t Allocator::getUsedSpace(bool newSpace) {
	size_t i;  
	size_t usedSpace = 0;
	if (newSpace) {
		for (i = newSpaceStartHeapIndex; i < newSpaceEndHeapIndex; i++)
			if (isBitSet(i))
				usedSpace++;
	} else {
		for (i = oldSpaceStartHeapIndex; i < oldSpaceEndHeapIndex; i++)
			if (isBitSet(i))
				usedSpace++;
	}
	return (size_t) usedSpace;
}

void Allocator::moveObject(Object *object) {
	return;
}


void Allocator::initializeHeap(size_t heapSize) {
}

void Allocator::setAllocated(size_t heapIndex, size_t size) {
	size_t i;
	size_t toMark = heapIndex;

	for (i = 0; i < size; i++) {
		setBitUsed(toMark);
		toMark++;
	}
}

void Allocator::setFree(size_t heapIndex, size_t size) {
	size_t i;
	size_t toFree = heapIndex;

	for (i = 0; i < size; i++) {
		setBitUnused(toFree);
		toFree++;
	}
}

size_t Allocator::getHeapSize() {
	return overallHeapSize;
}

int Allocator::getRegionSize() {
	// this method can be generalized/overridden to support an arbitrary number of regions.
	return isSplitHeap ? overallHeapSize / 2 : overallHeapSize;
}


void Allocator::printMap() {
	fprintf(heapMap, "%7d", gLineInTrace);

	size_t i;
	for (i = 0; i < overallHeapSize; i++) {
		if (isBitSet(i) == 1) {
			fprintf(heapMap, "X");
		} else {
			fprintf(heapMap, "_");
		}
	}

	fprintf(heapMap, "\n");
}

/*inline*/ bool Allocator::isBitSet(size_t heapIndex) {
	size_t byteNR = heapIndex>>3;
	size_t bit = 7 - heapIndex % 8;
	return ((myHeapBitMap[byteNR] & (1 << bit))>0)?true:false;
}

void Allocator::setBitUsed(size_t heapIndex) {
	if (heapIndex > (size_t) overallHeapSize) {
		fprintf(stderr,
				"ERROR(Line %d): setBitUsed request to illegal slot %zu\n",
				gLineInTrace, heapIndex);
		exit(1);
	}

	size_t byte = heapIndex / 8;
	size_t bit = 7 - heapIndex % 8;

	myHeapBitMap[byte] = myHeapBitMap[byte] | 1 << bit;
}

void Allocator::setBitUnused(size_t heapIndex) {
	if (heapIndex > (size_t) overallHeapSize) {
		fprintf(stderr, "add %zu heap %zu\n", heapIndex, overallHeapSize);
		fprintf(stderr, "ERROR: setBitUnused request to illegal slot\n");
	}

	size_t byte = heapIndex / 8;
	size_t bit = 7 - heapIndex % 8;

	myHeapBitMap[byte] = myHeapBitMap[byte] & ~(1 << bit);
}

void Allocator::printStats() {
	if (DEBUG_MODE && WRITE_HEAPMAP) {
		printMap();
	}

	size_t bytesAllocated = overallHeapSize - getFreeSize();

	fprintf(allocLog, "%7d: alloc: %zu obj: %7d\n", gLineInTrace,
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

void *Allocator::allocate(size_t size, size_t lower, size_t upper) {
	return NULL;
}

void Allocator::printStats(long trigReason) {
}

Allocator::~Allocator() {
}

}
