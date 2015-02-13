/*
 * Allocator.hpp
 *
 *  Created on: 2013-09-03
 *      Author: GarCoSim
 */

#ifndef ALLOCATOR_HPP_
#define ALLOCATOR_HPP_

#include "Object.hpp"
#include <stdio.h>

namespace traceFileSimulator {

class Allocator {
public:
	Allocator(int heapSize);

	int gcAllocate(int size);
	void gcFree(Object* object);

	//used mainly by garbage collector
	int getFreeSize();
	int getHeapSize();
	void setAllocationSeearchStart(int address);

	//stats
	void printMap();
	void printStats();
	void freeAllSectors();

private:
	virtual ~Allocator();

	inline bool isBitSet(unsigned int address);
	void setBitUsed(unsigned int address);
	void setBitUnused(unsigned int address);
	void setAllocated(int address, int size);
	void setFree(int address, int size);

	char* myHeapBitMap;
	int myHeapSize;
	int statBytesAllocated;
	int statLiveObjects;
	FILE* allocLog;
	FILE* heapMap;
	int myLastSuccessAddress;
};

} 
#endif /* ALLOCATOR_HPP_ */
