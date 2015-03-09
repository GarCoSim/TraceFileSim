/*
 * SimulatedAllocator.cpp
 *
 *  Created on: 2013-09-03
 *      Author: GarCoSim
 */

#include "SimulatedAllocator.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <climits>
#include "../defines.hpp"
#include <string>

extern int gLineInTrace;

using namespace std;

namespace traceFileSimulator {

SimulatedAllocator::SimulatedAllocator() {
}

bool SimulatedAllocator::isRealAllocator() {
	return false;
}

void SimulatedAllocator::initializeHeap(int heapSize) {
	myHeapBitMap = new char[heapSize / 8 + 1];

	myHeapSizeOldSpace = heapSize;
	myLastSuccessAddressOldSpace = 0;
	myLastSuccessAddressNewSpace = heapSize / 2;
	myHeapSizeNewSpace = heapSize;

	statBytesAllocated = 0;
	statLiveObjects = 0;
	if (DEBUG_MODE && WRITE_ALLOCATION_INFO) {
		allocLog = fopen("alloc.log", "w+");
	}
	if (DEBUG_MODE && WRITE_HEAPMAP) {
		heapMap = fopen("heapmap.log", "w+");
	}
	newSpaceOffset = heapSize / 2;
	oldSpaceOffset = 0;
	overallHeapSize = heapSize;
}

void SimulatedAllocator::moveObject(Object *object) {
	int size = object->getPayloadSize();
	gcFree(object); // first we need to reclaim the old space
	size_t address = (size_t)allocateInNewSpace(size);
	if (address == (size_t)-1) {
		fprintf(stderr, "error moving object (size %d), old space %d, new space %d\n", size, getUsedSpace(false), getUsedSpace(true));
		exit(1);
	}

	object->updateAddress(address);
}

int SimulatedAllocator::getUsedSpace(bool newSpace) {
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

size_t SimulatedAllocator::allocateInNewSpace(int size) {
	size_t address = (size_t)allocate(size, newSpaceOffset, myHeapSizeNewSpace, myLastSuccessAddressNewSpace);
	if (address == (size_t)-1)
		return (size_t)-1;
	myLastSuccessAddressNewSpace = address;
	return myLastSuccessAddressNewSpace;
}

bool SimulatedAllocator::isInNewSpace(Object *object) {
	int address = object->getAddress();
	
	if (address >= newSpaceOffset && address < myHeapSizeNewSpace)
		return true;

	return false;
}

void SimulatedAllocator::swapHeaps() {
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

void SimulatedAllocator::freeAllSectors() {
	int i;

	for (i = 0; i < overallHeapSize; i++) {
		setBitUnused(i);
	}

	statBytesAllocated = 0;
}

size_t SimulatedAllocator::allocate(int size, int lower, int upper, int lastAddress) {
	if (size <= 0) {
		return -1;
	}

	//hope for the best, assume the worst
	int address = lower;
	int contiguous = 0;
	//nextFit search
	int i, bit;
	int passedBoundOnce = 0;
	int end = (lastAddress - 1);

	for (i = lastAddress; i != end; i++) {
		bit = isBitSet(i);

		if (i == upper) {
			if (passedBoundOnce == 1) {
				return -1;
			}
			i = lower;
			contiguous = 0;
			address = i + 1;
			passedBoundOnce = 1;
		}

		if (bit == 1) {
			address = i + 1;
			contiguous = 0;
		} else {
			contiguous++;
			if (contiguous == size) {
				setAllocated(address, size);
				statBytesAllocated += size;
				statLiveObjects++;
				return address;
			}
		}
	}

	return -1;
}

size_t SimulatedAllocator::gcAllocate(int size) {
	size_t address = allocate(size, oldSpaceOffset, myHeapSizeOldSpace, myLastSuccessAddressOldSpace);
	if (address == (size_t)-1)
		return (size_t)-1;
	myLastSuccessAddressOldSpace = address;
	return myLastSuccessAddressOldSpace;
}

void SimulatedAllocator::gcFree(Object* object) {
	int address = object->getAddress();
	int size = object->getPayloadSize();

	setFree(address, size);
	//object->setFreed(1);

	statLiveObjects--;
	statBytesAllocated -= size;
}

//used mainly by garbage collector
int SimulatedAllocator::getFreeSize() {
	return isSplitHeap ? (overallHeapSize / 2) - statBytesAllocated : overallHeapSize - statBytesAllocated;
}

void SimulatedAllocator::setAllocated(int address, int size) {
	int i;
	int pointer = address;

	for (i = 0; i < size; i++) {
		setBitUsed(pointer);
		pointer++;
	}
}

void SimulatedAllocator::setFree(int address, int size) {
	int i;
	int pointer = address;

	for (i = 0; i < size; i++) {
		setBitUnused(pointer);
		pointer++;
	}
}

int SimulatedAllocator::getHeapSize() {
	return isSplitHeap ? overallHeapSize / 2: overallHeapSize;
}

inline bool SimulatedAllocator::isBitSet(unsigned int address) {
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

void SimulatedAllocator::setBitUsed(unsigned int address) {
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

void SimulatedAllocator::setBitUnused(unsigned int address) {
	if (address > (unsigned int) overallHeapSize) {
		fprintf(stderr, "ERROR: setBitUnused request to illegal slot\n");
	}

	int byte = address / 8;
	int bit = 7 - address % 8;

	myHeapBitMap[byte] = myHeapBitMap[byte] & ~(1 << bit);
}

void SimulatedAllocator::printMap() {
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

void SimulatedAllocator::printStats() {
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

void SimulatedAllocator::setAllocationSeearchStart(int address) {
	if (address > overallHeapSize) {
		return;
	}

	myLastSuccessAddressOldSpace = address;
}

SimulatedAllocator::~SimulatedAllocator() {
}

}
