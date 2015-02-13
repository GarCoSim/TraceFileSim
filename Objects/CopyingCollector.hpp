/*
 * CopyingCollector.hpp
 *
 *  Created on: 2013-09-04
 *      Author: GarCoSim
 */

#ifndef CopyingCollector_HPP_
#define CopyingCollector_HPP_

#include "Allocator.hpp"
#include "ObjectContainer.hpp"
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

} 
#endif /* CopyingCollector_HPP_ */
