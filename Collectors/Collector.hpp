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
	virtual bool candidatesContainObj(Object *obj);
	virtual void removeObjectFromCandidates(Object *obj);

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

	std::map<int, int> traversalDepthObjects; //To keep track of depth per object. Needed to determine depths for children.
	std::map<int, int> traversalDepth; //Amount of objects per depth
	std::multimap<float,int> overallTraversalDepthStats; //Average depth coupled with amount of obejcts. Used to calculate the final stats.
	void printTraversalDepthStats();
	int amountRootObjects;
	int amountOtherObjects;
	void printRootCountStats();
	std::map<int, int> rootObjects; //To keep track of root count per object.
	std::multimap<float,int> overallRootStats; //Average roots coupled with amount of obejcts. Used to calculate the final root stats.

	std::set<Object *> deadObjectsLocked;
};

}
#endif /* COLLECTOR_HPP_ */
