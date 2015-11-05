/*
 * RegionBasedAllocator.hpp
 *
 *  Created on: 2015-11-03
 *      Author: GarCoSim
 *
 */

#ifndef _REGIONBASEDALLOCATOR_HPP_
#define _REGIONBASEDALLOCATOR_HPP_

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

class RegionBasedAllocator : public Allocator {
public:
	RegionBasedAllocator();
	virtual ~RegionBasedAllocator();

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
#endif /* _REGIONBASEDALLOCATOR_HPP_ */
