/*
 * RealAllocator.hpp
 *
 *  Created on: 2013-09-03
 *      Author: GarCoSim
 */

#ifndef _REALALLOCATOR_HPP_
#define _REALALLOCATOR_HPP_

#include "Allocator.hpp"
#include "../Main/Object.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <climits>
#include "../defines.hpp"
#include <memory.h>
#include <string>
//added by Tristan 
#include <math.h>
#include <sys/time.h> 
#include "../Main/CardTable.hpp" 
 
namespace traceFileSimulator {

class RealAllocator : public Allocator {
public:
	RealAllocator();
	virtual ~RealAllocator();

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
#endif /* _REALALLOCATOR_HPP_ */
