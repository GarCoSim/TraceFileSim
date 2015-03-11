/*
 * RealAllocator.cpp
 *
 *  Created on: 2015-03-04
 *      Author: GarCoSim
 */

#include "RealAllocator.hpp"

extern int gLineInTrace;

using namespace std;

namespace traceFileSimulator {

RealAllocator::RealAllocator() {
}

bool RealAllocator::isRealAllocator() {
	return true;
}

void RealAllocator::initializeHeap(int heapSize) {
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

	heap = (unsigned char*)malloc(heapSize * 8);
	myLastSuccessAddressOldSpace = (size_t)&heap[0];
	myLastSuccessAddressNewSpace = (size_t)&heap[0];
}

void RealAllocator::freeAllSectors() {
	int i;

	for (i = 0; i < overallHeapSize; i++) {
		setBitUnused(i);
	}

	statBytesAllocated = 0;
}

size_t RealAllocator::allocate(int size, int lower, int upper, size_t lastAddress) {
	if (size <= 0) {
		return -1;
	}

	//hope for the best, assume the worst
	int address = lower;
	int contiguous = 0;
	//nextFit search
	int i, bit;
	int passedBoundOnce = 0;
	//int end = (lastAddress - (size_t)(&heap) - 1);

	for (i = lower; i != upper; i++) {
	//for (i = lastAddress - (size_t)(&heap); i != end; i++) {
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
				return (size_t)&heap[address];
			}
		}
	}

	return -1;
}

void RealAllocator::gcFree(Object* object) {
	size_t address = (size_t)&object;
	int size = object->getPayloadSize();
	size_t i;

	// find the address we're looking for
	for (i = 0; i < (size_t)overallHeapSize; i++) {
		if (address == (size_t)&heap[i])
			break;
	}
	if (address == (size_t)&heap[i]) {
		fprintf(stderr, "could not find the address %zx, that shouldn't have happened\n", address);
	}

	setFree(i, size);
	//object->setFreed(1);

	statLiveObjects--;
	statBytesAllocated -= size;
}

RealAllocator::~RealAllocator() {
}

}
