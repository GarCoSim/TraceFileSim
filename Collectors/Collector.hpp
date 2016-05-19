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
#include "../WriteBarriers/WriteBarrier.hpp"
#include <queue>
#include <stack>
#include <map>
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
	void updatePointers();
	void addForwardingEntry(void *oldAddress, void *newAddress);
	void clearForwardingEntries();
	virtual void initializeHeap();
	virtual void freeObject(Object *obj);

	//Methods for the recycler
	virtual void addCandidate(Object *obj);
	virtual bool candidatesNotContainObj(Object *obj);
	virtual bool candidatesContainObj(Object *obj);
	virtual void removeObjectFromCandidates(Object *obj);

protected:
	void postCollect();

	Allocator* myAllocator;
	ObjectContainer* myObjectContainer;
	queue<Object *> myQueue;
	stack<Object *> myStack;
	map<void *, void *> forwardPointers;

	WriteBarrier* myWriteBarrier;
	
	int statGcNumber;
	int statFreedObjects;
	int statLiveObjectCount;
	int statFreeSpaceOnHeap;
	int statFreeSpaceFragmentCount;
	int statCollectionReason;
	size_t myWatermark;
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
