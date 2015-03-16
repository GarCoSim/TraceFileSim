/*
 * SimulatedAllocator.cpp
 *
 *  Created on: 2013-09-03
 *      Author: GarCoSim
 */

#include "SimulatedAllocator.hpp"

extern int gLineInTrace;

using namespace std;

namespace traceFileSimulator {

SimulatedAllocator::SimulatedAllocator() {
}

bool SimulatedAllocator::isRealAllocator() {
	return false;
}

void SimulatedAllocator::freeAllSectors() {
	int i;

	for (i = 0; i < overallHeapSize; i++) {
		setBitUnused(i);
	}

	statBytesAllocated = 0;
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

size_t SimulatedAllocator::allocate(int size, int lower, int upper, size_t lastAddress) {
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

bool SimulatedAllocator::isInNewSpace(Object *object) {
	int address = object->getAddress();
	
	if (address >= newSpaceOffset && address < myHeapSizeNewSpace)
		return true;

	return false;
}

void SimulatedAllocator::moveObject(Object *object) {
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

void SimulatedAllocator::gcFree(Object* object) {
	int address = object->getAddress();
	int size = object->getPayloadSize();

	setFree(address, size);
	//object->setFreed(1);

	statLiveObjects--;
	statBytesAllocated -= size;
}

SimulatedAllocator::~SimulatedAllocator() {
}

}
