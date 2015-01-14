/*
 * Allocator.cpp
 *
 *  Created on: 2013-09-03
 *      Author: kons
 */

#include "Allocator.h"
#include <stdio.h>
#include <stdlib.h>
#include <climits>
#include "../defines.h"
//for stats
#include <string>

extern int gLineInTrace;

using namespace std;
namespace traceFileSimulator {

Allocator::Allocator(int heapSize) {
	myHeapBitMap = new char[heapSize / 8 + 1];
	myHeapSize = heapSize;
	myLastSuccessAddress = 0;

	statBytesAllocated = 0;
	statLiveObjects = 0;
	if (DEBUG_MODE == 1 && WRITE_ALLOCATION_INFO == 1) {
		allocLog = fopen("alloc.log", "w+");
	}
	if (DEBUG_MODE == 1 && WRITE_HEAPMAP == 1) {
		heapMap = fopen("heapmap.log", "w+");
	}

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

int Allocator::gcFree(Object* object) {
	int address = object->getAddress();
	int size = object->getPayloadSize();

	setFree(address, size);
	//object->setFreed(1);

	statLiveObjects--;
	statBytesAllocated -= size;
	return 1;
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

/*YAY, bit operation functions ( All nice and private in this class) */

inline int Allocator::isBitSet(unsigned int address) {
	if (address > (unsigned int) myHeapSize) {
		fprintf(stderr, "ERROR(Line %d): isBitSet request to illegal slot %d\n",
				gLineInTrace, address);
	}
	int byteNR = address / 8;
	if ((unsigned char) myHeapBitMap[byteNR] == 255) {
		return 1;
	}

	int bit = 7 - address % 8;
	int value = myHeapBitMap[byteNR] & 1 << bit;
	if (value > 0)
		return 1;
	else
		return 0;
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
//	char result = myHeapBitMap[byte];
//	int i = 1;
}

void Allocator::setBitUnused(unsigned int address) {
	if (address > (unsigned int) myHeapSize) {
		fprintf(stderr, "ERROR: setBitUnused request to illegal slot\n");
	}
	int byte = address / 8;
	int bit = 7 - address % 8;

	myHeapBitMap[byte] = myHeapBitMap[byte] & ~(1 << bit);
//	char result = myHeapBitMap[byte];
//	int i = 1;
}

void Allocator::printMap() {
	//filename.insert(filename.begin(), (char)gcCount);
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
	if (DEBUG_MODE == 1 && WRITE_HEAPMAP == 1) {
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

} /* namespace gcKons */
