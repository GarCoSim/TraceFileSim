/*
 * RegionBased.hpp
 *
 *  Created on: 2015-09-04
 *      Author: GarCoSim
 *
 * This collector implements a next-fit allocation policy.
 */

#ifndef _REGIONBASED_HPP_
#define _REGIONBASED_HPP_

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
#include "../Main/Region.hpp"

#define  THREADBASED 1
 
namespace traceFileSimulator {

class RegionBased : public Allocator {
public:
	RegionBased();
	virtual ~RegionBased();

	bool isRealAllocator();
	void freeAllSectors();
	void gcFree(Object* object);
	void initializeHeap(int heapSize);
	void moveObject(Object *object);
	bool isInNewSpace(Object *object);
	void freeOldSpace();
	void initRegions(int heapSize);
	void printStats(long trigReason);

private:
	void *allocate(int size, int lower, int upper);
	void *allocate(int size, int lower, int upper, int thread);
	unsigned int getHeapIndex(Object *object);

	unsigned char *heap;

	std::vector<int>  freeList;
};

} 
#endif /* _REGIONBASED_HPP_ */
