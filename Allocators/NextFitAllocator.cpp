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

	size_t potentialStart, contiguous = 0;
	bool hasWrappedAround = false;
	size_t tempStart = 0;
	if(oldSpaceRememberedHeapIndex==oldSpaceStartHeapIndex){
		tempStart = oldSpaceStartHeapIndex;
	}
	else if(oldSpaceRememberedHeapIndex==newSpaceStartHeapIndex){
		tempStart = newSpaceStartHeapIndex;
	}
	else{
		tempStart = oldSpaceRememberedHeapIndex+1;
	}
	
	for (potentialStart = tempStart; !hasWrappedAround || potentialStart<=(size_t)oldSpaceRememberedHeapIndex; potentialStart++) {
		if (potentialStart > upper) {
			hasWrappedAround = true;
			potentialStart = lower;
			contiguous = 0;
		}

		bool bitSet = false;
		if (isBitSet(potentialStart)){
			continue;
		}

		for (contiguous=0; contiguous<size; contiguous++) {
			if (potentialStart+contiguous > upper || isBitSet(potentialStart+contiguous)){
				bitSet = true;
				break;
			}
		}
		if (contiguous == size && bitSet == false && potentialStart+contiguous<=upper) { // found a free slot big enough
			oldSpaceRememberedHeapIndex = potentialStart;
			setAllocated(potentialStart, size);
			return &heap[potentialStart];
		}
		potentialStart+=contiguous;
	}

	return NULL;
}


NextFitAllocator::~NextFitAllocator() {
}

}
