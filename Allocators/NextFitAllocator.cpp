/*
 * NextFitAllocator.hpp
 *
 *  Created on: 2015-09-04
 *      Author: GarCoSim
 *
 * This collector implements a simple first-fit allocation policy.
 */

#include "NextFitAllocator.hpp"

extern int gLineInTrace;
using namespace std;


namespace traceFileSimulator {

NextFitAllocator::NextFitAllocator() {
}

bool NextFitAllocator::isRealAllocator() {
	return true;
}

bool NextFitAllocator::isInNewSpace(Object *object) {
	size_t heapIndex = getHeapIndex(object);
	return heapIndex >= newSpaceStartHeapIndex && heapIndex < newSpaceEndHeapIndex; 
}


void NextFitAllocator::moveObject(Object *object) {
	if (isInNewSpace(object))
		return;

	size_t size = object->getHeapSize();
	size_t address = (size_t)allocateInNewSpace(size);

	if (address == (size_t)-1) {
		fprintf(stderr, "error moving object (size %zu) with id %d, old space %zu, new space %zu\n", size, object->getID(), getUsedSpace(false), getUsedSpace(true));
		exit(1);
	}
	memcpy((void *) address, (void *) object->getAddress(), size);

	object->updateAddress((void *) address);
	object->setForwarded(true);
}

void NextFitAllocator::initializeHeap(size_t heapSize) {
	overallHeapSize = heapSize;
	myHeapBitMap = new char[(size_t)ceil(heapSize/8.0) ];
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

void NextFitAllocator::freeAllSectors() {
	size_t i;
	for (i = 0; i < overallHeapSize; i++) {
		setBitUnused(i);
	}

}

void *NextFitAllocator::allocate(size_t size, size_t lower, size_t upper) {
	if (size <= 0)
		return NULL;

	if ((size_t) oldSpaceRememberedHeapIndex < lower || (size_t) oldSpaceRememberedHeapIndex > upper)
		oldSpaceRememberedHeapIndex = lower; // essentially fall back to first fit

	size_t potentialStart, contiguous = 1;
	bool hasWrappedAround = false;
	for (potentialStart=oldSpaceRememberedHeapIndex+1; !hasWrappedAround || potentialStart<=(size_t)oldSpaceRememberedHeapIndex; potentialStart+=contiguous) {
		if (potentialStart > upper) {
			hasWrappedAround = true;
			potentialStart = lower;
			contiguous = 1;
		}

		if (isBitSet(potentialStart))
			continue;

		for (contiguous=1; contiguous<size; contiguous++) {
			if (potentialStart+contiguous > upper || isBitSet(potentialStart+contiguous))
				break;
		}
		if (contiguous == size) { // found a free slot big enough
			oldSpaceRememberedHeapIndex = potentialStart;
			setAllocated(potentialStart, size);
			return &heap[potentialStart];
		}
	}

	return NULL;
}

size_t NextFitAllocator::getHeapIndex(Object *object) {
	// This conversion is only valid because the heap is an array of bytes.
	return (size_t) ((char *) object->getAddress() - (char *) heap);
}


void NextFitAllocator::gcFree(Object* object) {
	size_t size = object->getHeapSize();
	size_t heapIndex = getHeapIndex(object);

	setFree(heapIndex, size);
	statLiveObjects--;
}

NextFitAllocator::~NextFitAllocator() {
}

void NextFitAllocator::freeOldSpace() {
	setFree(oldSpaceStartHeapIndex, oldSpaceEndHeapIndex-oldSpaceStartHeapIndex);
}

}
