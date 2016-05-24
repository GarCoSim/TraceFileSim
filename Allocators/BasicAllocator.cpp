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

void *BasicAllocator::allocate(size_t size, size_t lower, size_t upper) {
	if (size <= 0 || size > upper){
		return NULL;
	}

	size_t potentialStart, contiguous;
	for (potentialStart=lower; potentialStart <= upper-size; potentialStart++) {

		for (contiguous=0; contiguous<size; contiguous++) {
			if (isBitSet(potentialStart+contiguous)){
				break;
			}
		}
		if (contiguous == size) { // found a free slot big enough
			setAllocated(potentialStart, size);
			return &heap[potentialStart];
		}
	}

	return NULL;
}


BasicAllocator::~BasicAllocator() {
}

}
