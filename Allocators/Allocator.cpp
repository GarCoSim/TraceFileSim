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
		myHeapSizeOldSpace = overallHeapSize / 2;
		newSpaceOffset = myHeapSizeOldSpace;
		oldSpaceOffset = 0;
		isSplitHeap = true;
	}
	else {
		myHeapSizeOldSpace = overallHeapSize;
		isSplitHeap = false;
	}
}

Allocator::Allocator() {
}

int Allocator::getFreeSize() {
	return isSplitHeap ? (overallHeapSize / 2) - statBytesAllocated : overallHeapSize - statBytesAllocated;
}


size_t Allocator::gcAllocate(int size) {
	size_t address = allocate(size, oldSpaceOffset, myHeapSizeOldSpace, myLastSuccessAddressOldSpace);
	if (address == (size_t)-1)
		return (size_t)-1;
	myLastSuccessAddressOldSpace = address;
	return myLastSuccessAddressOldSpace;
}

size_t Allocator::allocateInNewSpace(int size) {
	size_t address = (size_t)allocate(size, newSpaceOffset, myHeapSizeNewSpace, myLastSuccessAddressNewSpace);
	if (address == (size_t)-1)
		return (size_t)-1;
	myLastSuccessAddressNewSpace = address;
	return myLastSuccessAddressNewSpace;
}

bool Allocator::isInNewSpace(Object *object) {
	int address = object->getAddress();
	
	if (address >= newSpaceOffset && address < myHeapSizeNewSpace)
		return true;

	return false;
}

void Allocator::swapHeaps() {
	int temp;

	myHeapSizeNewSpace = myHeapSizeOldSpace;
	oldSpaceOffset = newSpaceOffset;
	if (newSpaceOffset == 0) {
		myHeapSizeOldSpace = overallHeapSize / 2;
		newSpaceOffset = overallHeapSize / 2;
	} else {
		myHeapSizeOldSpace = overallHeapSize;
		newSpaceOffset = 0;
	}

	temp = myLastSuccessAddressOldSpace;
	myLastSuccessAddressOldSpace = myLastSuccessAddressNewSpace;
	myLastSuccessAddressNewSpace = temp;
}

int Allocator::getUsedSpace(bool newSpace) {
	int i, usedSpace = 0;
	if (newSpace) {
		for (i = newSpaceOffset; i < myHeapSizeNewSpace; i++)
			if (isBitSet(i))
				usedSpace++;
	} else {
		for (i = oldSpaceOffset; i < myHeapSizeOldSpace; i++)
			if (isBitSet(i))
				usedSpace++;
	}
	return usedSpace;
}

void Allocator::moveObject(Object *object) {
	if (isInNewSpace(object))
		return;

	int size = object->getPayloadSize();
	gcFree(object); // first we need to reclaim the old space
	size_t address = (size_t)allocateInNewSpace(size);
	if (address == (size_t)-1) {
		fprintf(stderr, "error moving object (size %d), old space %d, new space %d\n", size, getUsedSpace(false), getUsedSpace(true));
		exit(1);
	}

	object->updateAddress(address);
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
	return isSplitHeap ? overallHeapSize / 2: overallHeapSize;
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
	if (address > (unsigned int) overallHeapSize) {
		fprintf(stderr, "ERROR(Line %d): isBitSet request to illegal slot %d\n",
				gLineInTrace, address);
	}

	int byteNR = address / 8;

	if ((unsigned char) myHeapBitMap[byteNR] == 255) {
		return true;
	}

	int bit = 7 - address % 8;
	int value = myHeapBitMap[byteNR] & 1 << bit;

	if (value > 0) {
		return true;
	} else {
		return false;
	}
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

void Allocator::setAllocationSeearchStart(int address) {
	if (address > overallHeapSize) {
		return;
	}

	myLastSuccessAddressOldSpace = address;
}

bool Allocator::isRealAllocator() {
	return false;
}

void Allocator::freeAllSectors() {
}

void Allocator::gcFree(Object* object) {
}

size_t Allocator::allocate(int size, int lower, int upper, size_t lastAddress) {
	return -1;
}

Allocator::~Allocator() {
}

}
