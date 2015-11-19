/*
 * ThreadBasedAllocator.hpp
 *
 *  Created on: 2015-09-04
 *      Author: GarCoSim
 *
 */

#ifndef _THREADBASEDALLOCATOR_HPP_
#define _THREADBASEDALLOCATOR_HPP_

#include "Allocator.hpp"
#include "../Main/Object.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <climits>
#include "../defines.hpp"
#include <memory.h>
#include <math.h>
#include <string>
#include <sys/time.h> 
#include "../Main/ThreadOwnedRegion.hpp"

namespace traceFileSimulator {

class ThreadBasedAllocator : public Allocator {
public:
	ThreadBasedAllocator();
	virtual ~ThreadBasedAllocator();

	bool isRealAllocator();
	void freeAllSectors();
	void gcFree(Object* object);
	void initializeHeap(size_t heapSize);
	void moveObject(Object *object);
	bool isInNewSpace(Object *object);
	void freeOldSpace();
	void initRegions(size_t heapSize);

private:
	void *allocate(size_t size, size_t lower, size_t upper);
	void *allocate(size_t size, size_t lower, size_t upper, int thread);
	size_t getHeapIndex(Object *object);

	unsigned char *heap;
};

} 
#endif /* _THREADBASEDALLOCATOR_HPP_ */
