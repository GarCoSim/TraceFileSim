/*
 * BasicAllocator.hpp
 *
 *  Created on: 2015-09-04
 *      Author: GarCoSim
 *
 * This collector implements a simple first-fit allocation policy.
 */

#include "BasicAllocator.hpp"

extern int gLineInTrace;
using namespace std;


namespace traceFileSimulator {

BasicAllocator::BasicAllocator() {
}

bool BasicAllocator::isRealAllocator() {
	return true;
}

bool BasicAllocator::isInNewSpace(Object *object) {
	unsigned int heapIndex = getHeapIndex(object);
	return heapIndex >= newSpaceStartHeapIndex && heapIndex < newSpaceEndHeapIndex; 
}


void BasicAllocator::moveObject(Object *object) {
	if (isInNewSpace(object))
		return;

	int size = object->getHeapSize();
	size_t address = (size_t)allocateInNewSpace(size);

	if (address == (size_t)-1) {
		fprintf(stderr, "error moving object (size %d) with id %d, old space %d, new space %d\n", size, object->getID(), getUsedSpace(false), getUsedSpace(true));
		exit(1);
	}
	memcpy((void *) address, (void *) object->getAddress(), size);

	object->updateAddress((void *) address);
	object->setForwarded(true);
}

void BasicAllocator::initializeHeap(int heapSize) {
	overallHeapSize = heapSize;
	myHeapBitMap = new char[(int)ceil(heapSize/8.0) ];
	heap = (unsigned char*)malloc(heapSize * 8);
	statLiveObjects = 0;
	resetRememberedAllocationSearchPoint();

	if (DEBUG_MODE && WRITE_ALLOCATION_INFO) {
		allocLog = fopen("alloc.log", "w+");
	}
	if (DEBUG_MODE && WRITE_HEAPMAP) {
		heapMap = fopen("heapmap.log", "w+");
	}
	fprintf(stderr, "heap size %zd\n", overallHeapSize);
}

void BasicAllocator::freeAllSectors() {
	unsigned int i;
	for (i = 0; i < overallHeapSize; i++) {
		setBitUnused(i);
	}

}

void *BasicAllocator::allocate(int size, int lower, int upper) {
	if (size <= 0)
		return NULL;

	int potentialStart, contiguous;
	for (potentialStart=lower; potentialStart < upper-size; potentialStart++) {
		if (isBitSet(potentialStart))
			continue;

		for (contiguous=1; contiguous<size; contiguous++) {
			if (isBitSet(potentialStart+contiguous))
				break;
		}
		if (contiguous == size) { // found a free slot big enough
			setAllocated(potentialStart, size);
			return &heap[potentialStart];
		}
	}

	return NULL;
}

unsigned int BasicAllocator::getHeapIndex(Object *object) {
	// This conversion is only valid because the heap is an array of bytes.
	return (unsigned int) ((char *) object->getAddress() - (char *) heap);
}


void BasicAllocator::gcFree(Object* object) {
	int size = object->getHeapSize();
	unsigned int heapIndex = getHeapIndex(object);

	setFree(heapIndex, size);
	statLiveObjects--;
}

BasicAllocator::~BasicAllocator() {
}

void BasicAllocator::freeOldSpace() {
	setFree(oldSpaceStartHeapIndex, oldSpaceEndHeapIndex-oldSpaceStartHeapIndex);
}

}
