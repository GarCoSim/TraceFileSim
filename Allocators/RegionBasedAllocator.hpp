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
	void initializeHeap(size_t heapSize);
	void moveObject(Object *object);
	bool isInNewSpace(Object *object);
	void freeOldSpace();

private:
	void *allocate(size_t size, size_t lower, size_t upper);
	size_t getHeapIndex(Object *object);

	//unsigned char *heap; //now in Allocator.hpp
};

} 
#endif /* _REGIONBASEDALLOCATOR_HPP_ */
