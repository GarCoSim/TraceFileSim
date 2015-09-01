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

int Allocator::getFreeSize() {
	return isSplitHeap ? (overallHeapSize / 2) - statBytesAllocated : overallHeapSize - statBytesAllocated;
}


void *Allocator::gcAllocate(int size) {
	void *address = allocate(size, oldSpaceStartHeapIndex, oldSpaceEndHeapIndex, myLastSuccessAddressOldSpace);
	if (address == NULL)
		return NULL;
	myLastSuccessAddressOldSpace = address;
	return myLastSuccessAddressOldSpace;
}

void *Allocator::allocateInNewSpace(int size) {
	void *address = allocate(size, newSpaceStartHeapIndex, newSpaceEndHeapIndex, myLastSuccessAddressNewSpace);
	if (address == NULL)
		return NULL;
	myLastSuccessAddressNewSpace = address;
	return myLastSuccessAddressNewSpace;
}

bool Allocator::isInNewSpace(Object *object) {
	return false;
}

void Allocator::swapHeaps() {
	unsigned int tempIndex;
	void *tempPtr;

	tempIndex = newSpaceStartHeapIndex;
	newSpaceStartHeapIndex = oldSpaceStartHeapIndex;
	oldSpaceStartHeapIndex = tempIndex;

	tempIndex = newSpaceEndHeapIndex;
	newSpaceEndHeapIndex = oldSpaceEndHeapIndex;
	oldSpaceEndHeapIndex = tempIndex;

	tempPtr = myLastSuccessAddressNewSpace;
	myLastSuccessAddressNewSpace = myLastSuccessAddressOldSpace;
	myLastSuccessAddressOldSpace = tempPtr;
}

void Allocator::freeOldSpace() {
	setFree(oldSpaceStartHeapIndex, oldSpaceEndHeapIndex-oldSpaceStartHeapIndex);
}

int Allocator::getUsedSpace(bool newSpace) {
	int i, usedSpace = 0;
	if (newSpace) {
		for (i = newSpaceStartHeapIndex; i < newSpaceEndHeapIndex; i++)
			if (isBitSet(i))
				usedSpace++;
	} else {
		for (i = oldSpaceStartHeapIndex; i < oldSpaceEndHeapIndex; i++)
			if (isBitSet(i))
				usedSpace++;
	}
	return usedSpace;
}

void Allocator::moveObject(Object *object) {
	return;
}


void Allocator::initializeHeap(int heapSize) {
}

void Allocator::setAllocated(int address, int size) {
	int i;
	int pointer = address;

	for (i = 0; i < size; i++) {
		setBitUsed(pointer);
		pointer++;
	}
}

void Allocator::setFree(int address, int size) {
	int i;
	int pointer = address;

	for (i = 0; i < size; i++) {
		setBitUnused(pointer);
		pointer++;
	}
}

int Allocator::getHeapSize() {
	return isSplitHeap ? overallHeapSize / 2 : overallHeapSize;
}

void Allocator::printMap() {
	fprintf(heapMap, "%7d", gLineInTrace);

	int i;

	for (i = 0; i < overallHeapSize; i++) {
		if (isBitSet(i) == 1) {
			fprintf(heapMap, "X");
		} else {
			fprintf(heapMap, "_");
		}
	}

	fprintf(heapMap, "\n");
}

inline bool Allocator::isBitSet(unsigned int address) {
	/*
	if (address > (unsigned int) overallHeapSize) {
		fprintf(stderr, "ERROR(Line %d): isBitSet request to illegal slot %d\n",
				gLineInTrace, address);
	}
*/
	int byteNR = address>>3;
	int bit    = 7 - address % 8;

    return ((myHeapBitMap[byteNR] & (1 << bit))>0)?true:false;
}

void Allocator::setBitUsed(unsigned int address) {
	if (address > (unsigned int) overallHeapSize) {
		fprintf(stderr,
				"ERROR(Line %d): setBitUsed request to illegal slot %d\n",
				gLineInTrace, address);
		exit(1);
	}

	int byte = address / 8;
	int bit = 7 - address % 8;

	myHeapBitMap[byte] = myHeapBitMap[byte] | 1 << bit;
}

void Allocator::setBitUnused(unsigned int address) {
	if (address > (unsigned int) overallHeapSize) {
		fprintf(stderr, "add %d heap %d\n", address, overallHeapSize);
		fprintf(stderr, "ERROR: setBitUnused request to illegal slot\n");
	}

	int byte = address / 8;
	int bit = 7 - address % 8;

	myHeapBitMap[byte] = myHeapBitMap[byte] & ~(1 << bit);
}

void Allocator::printStats() {
	if (DEBUG_MODE && WRITE_HEAPMAP) {
		printMap();
	}

	int bytesAllocated = 0;

	//traverse all heap and count allocated bits
	int i;

	for (i = 0; i < overallHeapSize; i++) {
		if (isBitSet(i) == 1) {
			bytesAllocated++;
		}
	}

	fprintf(allocLog, "%7d: alloc: %7d obj: %7d\n", gLineInTrace,
			bytesAllocated, statLiveObjects);
}

void Allocator::setAllocationSearchStart(int address) {
	if (address > overallHeapSize) {
		return;
	}

	// TODO this is just plain wrong, address is an index into the heap,
	// myLastSuccessAddressOldSpace is a pointer.

	// myLastSuccessAddressOldSpace = address;
}

bool Allocator::isRealAllocator() {
	return false;
}

void Allocator::freeAllSectors() {
}

void Allocator::gcFree(Object* object) {
}

void *Allocator::allocate(int size, int lower, int upper, void *lastAddress) {
	return NULL;
}

Allocator::~Allocator() {
}

}
