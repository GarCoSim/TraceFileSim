/*
 * MarkSweepCollector.h
 *
 *  Created on: 2013-09-04
 *      Author: kons
 */

#ifndef MARKSWEEPCOLLECTOR_H_
#define MARKSWEEPCOLLECTOR_H_

//#include "GarbageCollector.h"
#include "Allocator.h"
#include "ObjectContainer.h"
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
	Allocator* myAllocator;
	ObjectContainer* myObjectContainer;
	queue<Object *> myQueue;
	void mark();
	void sweep();
	void compact();
	void enqueueAllRoots();
	void initializeMarkPhase();
	void preCollect();
	void postCollect();
	void freeAllLiveObjects();
	void reallocateAllLiveObjects();
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

} /* namespace gcKons */
#endif /* MARKSWEEPCOLLECTOR_H_ */
