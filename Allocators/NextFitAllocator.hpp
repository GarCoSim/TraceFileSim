/*
 * NextFitAllocator.hpp
 *
 *  Created on: 2015-09-04
 *      Author: GarCoSim
 *
 * This collector implements a next-fit allocation policy.
 */

#ifndef _NEXTFITALLOCATOR_HPP_
#define _NEXTFITALLOCATOR_HPP_

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
 
namespace traceFileSimulator {

class NextFitAllocator : public Allocator {
public:
	NextFitAllocator();
	virtual ~NextFitAllocator();

	bool isRealAllocator();
	void freeAllSectors();
	void gcFree(Object* object);
	void initializeHeap(size_t heapSize);
	void moveObject(Object *object);
	bool isInNewSpace(Object *object);
	void freeOldSpace();

private:
	void *allocate(size_t size, size_t lower, size_t upper);
	size_t getHeapIndex(Object *object);

	unsigned char *heap;
};

} 
#endif /* _NEXTFITALLOCATOR_HPP_ */
