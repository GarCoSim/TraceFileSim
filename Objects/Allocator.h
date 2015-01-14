/*
 * Allocator.h
 *
 *  Created on: 2013-09-03
 *      Author: kons
 */

#ifndef ALLOCATOR_H_
#define ALLOCATOR_H_

#include "Object.h"
#include <stdio.h>

namespace traceFileSimulator {

class Allocator {
public:
	Allocator(int heapSize);
	int gcAllocate(int size);
	int gcFree(Object* object);

	//used mainly by garbage collector
	int getFreeSize();
	int getHeapSize();
	void setAllocationSeearchStart(int address);
	//stats
	void printMap();
	void printStats();
	void freeAllSectors();
private:
	char* myHeapBitMap;
	int myHeapSize;
	inline int isBitSet(unsigned int address);
	void setBitUsed(unsigned int address);
	void setBitUnused(unsigned int address);
	void setAllocated(int address, int size);
	void setFree(int address, int size);
	int statBytesAllocated;
	int statLiveObjects;
	FILE* allocLog;
	FILE* heapMap;
	int myLastSuccessAddress;
	virtual ~Allocator();
};

} /* namespace gcKons */
#endif /* ALLOCATOR_H_ */
