/*
 * BasicAllocator.hpp
 *
 *  Created on: 2015-09-04
 *      Author: GarCoSim
 *
 * This collector implements a simple first-fit allocation policy.
 */

#ifndef _BASICALLOCATOR_HPP_
#define _BASICALLOCATOR_HPP_

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

class BasicAllocator : public Allocator {
public:
	BasicAllocator();
	virtual ~BasicAllocator();

	bool isRealAllocator();
	void freeAllSectors();
	void gcFree(Object* object);
	void initializeHeap(int heapSize);
	void moveObject(Object *object);
	bool isInNewSpace(Object *object);
	void freeOldSpace();

private:
	void *allocate(int size, int lower, int upper);
	unsigned int getHeapIndex(Object *object);

	unsigned char *heap;
};

} 
#endif /* _BASICALLOCATOR_HPP_ */
