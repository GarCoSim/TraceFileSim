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


NextFitAllocator::~NextFitAllocator() {
}

}
