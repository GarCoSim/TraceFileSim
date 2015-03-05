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

namespace traceFileSimulator {

class RealAllocator : public Allocator {
public:
	RealAllocator();
	virtual ~RealAllocator();

	size_t gcAllocate(int size);
	void gcFree(Object* object);

	//used mainly by garbage collector
	int getFreeSize();
	int getHeapSize();
	void setAllocationSeearchStart(int address);

	//stats
	void printMap();
	void printStats();
	void freeAllSectors();

	void setHalfHeapSize(bool value);
	void moveObject(Object *object);
	void swapHeaps();

	bool isInNewSpace(Object *object);

	void initializeHeap(int heapSize);

	bool isRealAllocator();

private:
	inline bool isBitSet(unsigned int address);
	void setBitUsed(unsigned int address);
	void setBitUnused(unsigned int address);
	void setAllocated(int address, int size);
	void setFree(int address, int size);
	int allocateInNewSpace(int size);

	unsigned char *heap;

	int myHeapSizeNewSpace;
};

} 
#endif /* _REALALLOCATOR_HPP_ */
