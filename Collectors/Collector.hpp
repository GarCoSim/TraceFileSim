/*
 * Collector.hpp
 *
 *  Created on: 2013-09-04
 *      Author: GarCoSim
 */

#ifndef COLLECTOR_HPP_
#define COLLECTOR_HPP_

#include "../Allocators/Allocator.hpp"
#include "../Main/ObjectContainer.hpp"
#include <queue>
#include <stack>
#include <string>
#include <ctime>
#include "../defines.hpp"
#include <stdio.h>

using namespace std;

namespace traceFileSimulator {

class MemoryManager;

class Collector {
public:
	Collector();
	void setEnvironment(Allocator* allocator, ObjectContainer* container, MemoryManager* memManager, int watermark, int generation, int traversal);
	virtual ~Collector();
	virtual void collect(int reason);
	virtual void checkWatermark();
	void printStats();
	virtual int promotionPhase();
	void lastStats();

protected:
	void postCollect();

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
	int myTraversal;
	int statHeapSide;
	double shortestGC;
	double longestGC;
	double allGCs;
	
	MemoryManager* myMemManager;

	traversalEnum order;
};

} 
#endif /* COLLECTOR_HPP_ */
