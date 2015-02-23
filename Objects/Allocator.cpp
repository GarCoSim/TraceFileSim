/*
 * Allocator.cpp
 *
 *  Created on: 2013-09-03
 *      Author: GarCoSim
 */

#include "Allocator.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <climits>
#include "../defines.hpp"
#include <string>

extern int gLineInTrace;

using namespace std;

namespace traceFileSimulator {

Allocator::Allocator(int heapSize) {
	myHeapBitMap = new char[heapSize / 8 + 1];

	myHeapSize = heapSize;
	myLastSuccessAddress = 0;
	myLastSuccessAddressNewSpace = heapSize / 2;

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

}

void Allocator::setHalfHeapSize(bool value) {
	if (value)
		myHeapSize = overallHeapSize / 2;
	else
		myHeapSize = overallHeapSize;
}

void Allocator::moveObject(Object *object) {
	int size = object->getPayloadSize();
	object->updateAddress(allocateInNewSpace(size));
}

int Allocator::allocateInNewSpace(int size) {
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

		if (newSpaceOffset == 0) {
			if (i == myHeapSize) {
				if (passedBoundOnce == 1) {
					return -1;
				}
				i = 0;
				contiguous = 0;
				address = i + 1;
				passedBoundOnce = 1;
			}
		} else {
			if (i == overallHeapSize) {
				if (passedBoundOnce == 1) {
					return -1;
				}
				i = 0;
				contiguous = 0;
				address = i + 1;
				passedBoundOnce = 1;
			}
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
				return address;
			}
		}
	}

	return -1;
}

bool Allocator::isInNewSpace(Object *object) {
	int address = object->getAddress();
	
	if (newSpaceOffset == 0) {
		if (address >= 0 && address < overallHeapSize / 2)
			return true;
	} else {
		if (address >= overallHeapSize / 2 && address < overallHeapSize)
			return true;
	}

	return false;
}

void Allocator::swapHeaps() {
	int temp;

	newSpaceOffset = (newSpaceOffset == 0) ? overallHeapSize / 2 : 0;

	temp = myLastSuccessAddress;
	myLastSuccessAddress = myLastSuccessAddressNewSpace;
	myLastSuccessAddressNewSpace = temp;

	myHeapSize = newSpaceOffset;
}

void Allocator::freeAllSectors() {
	int i;

	for (i = 0; i < myHeapSize; i++) {
		setBitUnused(i);
	}

	statBytesAllocated = 0;
}

int Allocator::gcAllocate(int size) {
	if (size <= 0) {
		return -1;
	}

	//hope for the best, assume the worst
	int address = 0;
	int contiguous = 0;
	//nextFit search
	int i, bit;
	int passedBoundOnce = 0;
	int end = (myLastSuccessAddress - 1);

	for (i = myLastSuccessAddress; i != end; i++) {
		bit = isBitSet(i);

		if (i == myHeapSize) {
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
				myLastSuccessAddress = address;
				return address;
			}
		}
	}

	return -1;
}

void Allocator::gcFree(Object* object) {
	int address = object->getAddress();
	int size = object->getPayloadSize();

	setFree(address, size);
	//object->setFreed(1);

	statLiveObjects--;
	statBytesAllocated -= size;
}

//used mainly by garbage collector
int Allocator::getFreeSize() {
	return myHeapSize - statBytesAllocated;
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
	return myHeapSize;
}

inline bool Allocator::isBitSet(unsigned int address) {
	if (address > (unsigned int) myHeapSize) {
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
	if (address > (unsigned int) myHeapSize) {
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
	if (address > (unsigned int) myHeapSize) {
		fprintf(stderr, "ERROR: setBitUnused request to illegal slot\n");
	}

	int byte = address / 8;
	int bit = 7 - address % 8;

	myHeapBitMap[byte] = myHeapBitMap[byte] & ~(1 << bit);
}

void Allocator::printMap() {
	fprintf(heapMap, "%7d", gLineInTrace);

	int i;

	for (i = 0; i < myHeapSize; i++) {
		if (isBitSet(i) == 1) {
			fprintf(heapMap, "X");
		} else {
			fprintf(heapMap, "_");
		}
	}

	fprintf(heapMap, "\n");
}

void Allocator::printStats() {
	if (DEBUG_MODE && WRITE_HEAPMAP) {
		printMap();
	}

	int bytesAllocated = 0;

	//traverse all heap and count allocated bits
	int i;

	for (i = 0; i < myHeapSize; i++) {
		if (isBitSet(i) == 1) {
			bytesAllocated++;
		}
	}

	fprintf(allocLog, "%7d: alloc: %7d obj: %7d\n", gLineInTrace,
			bytesAllocated, statLiveObjects);
}

void Allocator::setAllocationSeearchStart(int address) {
	if (address > myHeapSize) {
		return;
	}

	myLastSuccessAddress = address;
}

Allocator::~Allocator() {
}

}
