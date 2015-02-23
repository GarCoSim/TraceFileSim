/*
 * Collector.hpp
 *
 *  Created on: 2013-09-04
 *      Author: GarCoSim
 */

#ifndef COLLECTOR_HPP_
#define COLLECTOR_HPP_

#include "Allocator.hpp"
#include "ObjectContainer.hpp"
#include <queue>
#include <stack>
#include "../defines.hpp"

using namespace std;

namespace traceFileSimulator {

class MemoryManager;

class Collector {
public:
	Collector();
	void setEnvironment(Allocator* allocator, ObjectContainer* container, MemoryManager* memManager, int watermark, int generation);
	virtual ~Collector();
	virtual void collect(int reason);
	virtual void checkWatermark();
	virtual void printStats();
	virtual int promotionPhase();

protected:
	Allocator* myAllocator;
	ObjectContainer* myObjectContainer;
	queue<Object *> myQueue;
	stack<Object *> myStack;
	
	int statGcNumber;
	int statFreedObjects;
	int statLiveObjectCount;
	int statFreeSpaceOnHeap;
	int statFreeSpaceFragmentCount;
	int statCollectionReason;
	int myWatermark;
	int myGeneration;
	int statFreedDuringThisGC;
	int gcsSinceLastPromotionPhase;
	int statHeapSide;
	
	MemoryManager* myMemManager;
};

} 
#endif /* COLLECTOR_HPP_ */
