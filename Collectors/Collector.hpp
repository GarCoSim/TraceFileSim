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
#include <set>
#include <vector>
#include "../defines.hpp"

using namespace std;

namespace traceFileSimulator {

class MemoryManager;
class Allocator;
class ObjectContainer;
class Object;
class WriteBarrier;

/** Abstract Class for collectors. Provides an implementable interface
 * for new collection algorithms.
 *
 */
class Collector {
public:
	Collector();
	void setEnvironment(Allocator* allocator, ObjectContainer* container, MemoryManager* memManager, size_t watermark, int generation, int traversal);
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

	void addDeadObjectLocked(Object *obj);
	std::set<Object *> getDeadObjectsLocked();
	void clearDeadObjectsLocked();

protected:
	void postCollect();
	void preCollect();
	void initializeMarkPhase();

	Allocator* myAllocator;
	ObjectContainer* myObjectContainer;
	queue<Object *> myQueue;
	stack<Object *> myStack;
	deque<Object *> myDoubleQueue;
	map<void *, void *> forwardPointers;

	WriteBarrier* myWriteBarrier;

	size_t statGcNumber;
	size_t statFreedObjects;
	size_t statLiveObjectCount;
	size_t statFreeSpaceOnHeap;
	size_t statFreeSpaceFragmentCount;
	int statCollectionReason;
	size_t myWatermark;
	int myGeneration;
	size_t statFreedDuringThisGC;
	size_t statCopiedDuringThisGC;
	size_t statCopiedObjects;
	size_t gcsSinceLastPromotionPhase;
	int myTraversal;
	int statHeapSide;
	double shortestGC;
	double longestGC;
	double allGCs;

	MemoryManager* myMemManager;

	traversalEnum order;

	std::map<int, int> traversalDepth;
	std::multimap<float,int> traversalDepthStats;
	void printTraversalDepthStats();
	int amountRootObjects;
	int amountOtherObjects;

	std::set<Object *> deadObjectsLocked;
};

}
#endif /* COLLECTOR_HPP_ */
