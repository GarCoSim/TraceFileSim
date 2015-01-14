/*
 * CopyingCollector.h
 *
 *  Created on: 2013-09-04
 *      Author: kons
 */

#ifndef CopyingCollector_H_
#define CopyingCollector_H_

//#include "GarbageCollector.h"
#include "Allocator.h"
#include "ObjectContainer.h"
#include <queue>

using namespace std;
namespace traceFileSimulator {

class CopyingCollector {
public:
	CopyingCollector(Allocator* allocator, ObjectContainer* container);
	virtual ~CopyingCollector();
	void collect(int reason);
//	void checkWatermark();
	void printStats();
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
	//indicates if this is the young generation part of a gen. gc
	int statGcNumber;
	int statFreedObjects;
	int statLiveObjectCount;
	int statFreeSpaceOnHeap;
	int statFreeSpaceFragmentCount;
	int statCollectionReason;
	int statFreedDuringThisGC;
	int statHeapSide;
};

} /* namespace gcKons */
#endif /* CopyingCollector_H_ */
