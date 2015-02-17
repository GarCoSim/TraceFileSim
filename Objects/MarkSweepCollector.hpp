/*
 * MarkSweepCollector.hpp
 *
 *  Created on: 2013-09-04
 *      Author: GarCoSim
 */

#ifndef MARKSWEEPCOLLECTOR_HPP_
#define MARKSWEEPCOLLECTOR_HPP_

#include "Allocator.hpp"
#include "ObjectContainer.hpp"
#include <queue>

using namespace std;

namespace traceFileSimulator {

class MemoryManager;

class MarkSweepCollector {
public:
	MarkSweepCollector(Allocator* allocator, ObjectContainer* container,
			MemoryManager* memManager, int watermark, int generation);
	virtual ~MarkSweepCollector();
	void collect(int reason);
	void checkWatermark();
	void printStats();
	int promotionPhase();

private:
	void mark();
	void sweep();
	void compact();
	void enqueueAllRoots();
	void initializeMarkPhase();
	void preCollect();
	void postCollect();
	void freeAllLiveObjects();
	void reallocateAllLiveObjects();

	Allocator* myAllocator;
	ObjectContainer* myObjectContainer;
	queue<Object *> myQueue;
	
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
	
	MemoryManager* myMemManager;
};

} 
#endif /* MARKSWEEPCOLLECTOR_HPP_ */
