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
	void initializeHeap(int heapSize);
	void moveObject(Object *object);
	bool isInNewSpace(Object *object);
	void freeOldSpace();
	void initRegions(int heapSize);

private:
	void *allocate(int size, int lower, int upper);
	void *allocate(int size, int lower, int upper, int thread);
	unsigned int getHeapIndex(Object *object);

	unsigned char *heap;
};

} 
#endif /* _THREADBASEDALLOCATOR_HPP_ */
