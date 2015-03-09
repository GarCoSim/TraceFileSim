/*
 * RealAllocator.cpp
 *
 *  Created on: 2015-03-04
 *      Author: GarCoSim
 */

#include "RealAllocator.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <climits>
#include "../defines.hpp"
#include <string>

extern int gLineInTrace;

using namespace std;

namespace traceFileSimulator {

RealAllocator::RealAllocator() {
}

void RealAllocator::initializeHeap(int heapSize) {
	myHeapBitMap = new char[heapSize / 8 + 1];

	myHeapSizeOldSpace = heapSize;
	myLastSuccessAddressOldSpace = 0;
	myLastSuccessAddressNewSpace = heapSize / 2;
	myHeapSizeNewSpace = overallHeapSize;

	statBytesAllocated = 0;
	statLiveObjects = 0;
	if (DEBUG_MODE && WRITE_ALLOCATION_INFO) {
		allocLog = fopen("alloc.log", "w+");
	}
	if (DEBUG_MODE && WRITE_HEAPMAP) {
		heapMap = fopen("heapmap.log", "w+");
	}
	newSpaceOffset = 0;
	overallHeapSize = heapSize;

	// here we do the actual instantiation of the heap, no simulating like in the other allocator :)
	heap = (unsigned char*)malloc(heapSize * 8);
}


void RealAllocator::moveObject(Object *object) {
	// if our object already survived we have nothing to do
	if (isInNewSpace(object))
		return;

	int size = object->getPayloadSize();
	object->updateAddress(allocateInNewSpace(size));
}

bool RealAllocator::isRealAllocator() {
	return true;
}


size_t RealAllocator::allocateInNewSpace(int size) {
	if (size <= 0) {
		return -1;
	}

	//hope for the best, assume the worst
	int address = 0;
	int contiguous = 0;
	//nextFit search
	int i, bit;
	int passedBoundOnce = 0;
	int end = (myLastSuccessAddressNewSpace - 1);

	for (i = myLastSuccessAddressNewSpace; i != end; i++) {
		bit = isBitSet(i);

		if (i == myHeapSizeNewSpace) {
			if (passedBoundOnce == 1) {
				return -1;
			}
			i = 0;
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
				myLastSuccessAddressNewSpace = address;
				return (size_t)&heap[address];
			}
		}
	}

	return -1;
}

bool RealAllocator::isInNewSpace(Object *object) {
	int address = object->getAddress();
	
	if (address >= newSpaceOffset && address < myHeapSizeNewSpace)
		return true;

	return false;
}

void RealAllocator::swapHeaps() {
	int temp;

	myHeapSizeNewSpace = myHeapSizeOldSpace;
	myHeapSizeOldSpace = (newSpaceOffset == 0) ? overallHeapSize : overallHeapSize / 2 + 1;

	newSpaceOffset = (newSpaceOffset == 0) ? overallHeapSize / 2 : 0;

	temp = myLastSuccessAddressOldSpace;
	myLastSuccessAddressOldSpace = myLastSuccessAddressNewSpace;
	myLastSuccessAddressNewSpace = temp;
}

void RealAllocator::freeAllSectors() {
	int i;

	for (i = 0; i < overallHeapSize; i++) {
		setBitUnused(i);
	}

	statBytesAllocated = 0;
}

size_t RealAllocator::gcAllocate(int size) {
	if (size <= 0) {
		return -1;
	}

	//hope for the best, assume the worst
	int address = 0;
	int contiguous = 0;
	//nextFit search
	int i, bit;
	int passedBoundOnce = 0;
	int end = (myLastSuccessAddressOldSpace - 1);

	for (i = myLastSuccessAddressOldSpace; i != end; i++) {
		bit = isBitSet(i);

		if (i == myHeapSizeOldSpace) {
			if (passedBoundOnce == 1) {
				return -1;
			}
			i = 0;
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
				myLastSuccessAddressOldSpace = address;
				return (size_t)&heap[address];
			}
		}
	}

	return -1;
}

void RealAllocator::gcFree(Object* object) {
	int address = object->getAddress();
	int size = object->getPayloadSize();

	setFree(address, size);
	//object->setFreed(1);

	statLiveObjects--;
	statBytesAllocated -= size;
}

//used mainly by garbage collector
int RealAllocator::getFreeSize() {
	return myHeapSizeOldSpace - statBytesAllocated;
}

void RealAllocator::setAllocated(int address, int size) {
	int i;
	int pointer = address;

	for (i = 0; i < size; i++) {
		setBitUsed(pointer);
		pointer++;
	}
}

void RealAllocator::setFree(int address, int size) {
	int i;
	int pointer = address;
	//unsigned char *heapPtr = &heap[address];

	for (i = 0; i < size; i++) {
		//*heapPtr++ = NULL; // overwrite the space
		setBitUnused(pointer);
		pointer++;
	}
}

int RealAllocator::getHeapSize() {
	return myHeapSizeOldSpace;
}

inline bool RealAllocator::isBitSet(unsigned int address) {
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

void RealAllocator::setBitUsed(unsigned int address) {
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

void RealAllocator::setBitUnused(unsigned int address) {
	if (address > (unsigned int) overallHeapSize) {
		fprintf(stderr, "ERROR: setBitUnused request to illegal slot\n");
	}

	int byte = address / 8;
	int bit = 7 - address % 8;

	myHeapBitMap[byte] = myHeapBitMap[byte] & ~(1 << bit);
}

void RealAllocator::printMap() {
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

void RealAllocator::printStats() {
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

void RealAllocator::setAllocationSeearchStart(int address) {
	if (address > overallHeapSize) {
		return;
	}

	myLastSuccessAddressOldSpace = address;
}

RealAllocator::~RealAllocator() {
}

}
