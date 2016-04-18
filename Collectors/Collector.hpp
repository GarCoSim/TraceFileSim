/*
 * Collector.hpp
 *
 *  Created on: 2013-09-04
 *      Author: GarCoSim
 */

#ifndef COLLECTOR_HPP_
#define COLLECTOR_HPP_

#include <queue>
#include <stack>
#include <deque>
#include <map>
#include "../defines.hpp"

using namespace std;

namespace traceFileSimulator {

class MemoryManager;
class Allocator;
class ObjectContainer;
class Object;
class WriteBarrier;

class Collector {
public:
	Collector();
	void setEnvironment(Allocator* allocator, ObjectContainer* container, MemoryManager* memManager, int watermark, int generation, int traversal);
	virtual ~Collector();
	virtual void collect(int reason) = 0;
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
	void preCollect();
	void compact();
	void initializeMarkPhase();
	void freeAllLiveObjects();
	void reallocateAllLiveObjects();

	Allocator* myAllocator;
	ObjectContainer* myObjectContainer;
	queue<Object *> myQueue;
	stack<Object *> myStack;
	deque<Object *> myDoubleQueue;
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
	int statCopiedDuringThisGC;
	int statCopiedObjects;
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
