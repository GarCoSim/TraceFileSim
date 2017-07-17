/*
 * TraversalCollector.hpp
 *
 *  Created on: 2015-10-27
 *      Author: GarCoSim
 */

#ifndef BALANCEDCOLLECTOR_HPP_
#define BALANCEDCOLLECTOR_HPP_

#include "Collector.hpp"
#include "../Allocators/Allocator.hpp"
#include "../Main/ObjectContainer.hpp"
#include "../Main/MemoryManager.hpp"
#include "../Main/Region.hpp"
#include "../defines.hpp"
#include <stack>
#include <queue>
#include <stdio.h>
#include <stdlib.h>
#include <ctime>

using namespace std;

namespace traceFileSimulator {

class MemoryManager;

/** Collector simulating the IBM Balanced GC policy
 *
 */
class BalancedCollector : public Collector {
public:
	BalancedCollector();
	virtual ~BalancedCollector();
	void collect(int reason);
	void initializeHeap();
	void addObjectToSpineRemset(Object* spine);

private:
	std::vector<unsigned int> myCollectionSet;
	std::vector<unsigned int> copyToRegions[MAXREGIONAGE+1];
	std::vector<Region*> allRegions;
	queue<Object *> myUpdatePointerQueue;
	queue<Object *> myPrintStatsQueue;
	void buildCollectionSet();
	void buildFinalCollectionSet();
	void preCollect();
	void calculateDeadSpace();
	void mark(Object* currentObject);
	int copy();
	void getRootObjects();
	void copyObjectsInQueues();
	void getRemsetObjects();
	int copyAndForwardObject(Object *obj);
	void updatePointers();
	void emptyHelpers();
	void removeObjects();
	void updateRemsetPointers();
	void printObjectInfo(Object *obj);
	void reOrganizeRegions();
	void printFinalStats();

	int totalObjectsInCollectionSet;

	//overloaded functions for thread-based GC
	std::vector<unsigned int> copyToRegionsTB[NUM_THREADS];

	void buildCollectionSet(int thread);
	int copy(int thread);
	int copyObjectsInQueues(int thread);
	int copyAndForwardObject(Object *obj,int thread);
	void emptyHelpers(int thread);
	void removeObjects(int regionIdx);
	void reOrganizeRegions(int regionIdx);
	size_t regionsInSet;
	size_t regionsReclaimed;

	typedef struct deadObjectStats{
		size_t regionID;
		size_t percentDead; //percentage of the region occupied by dead objects
	} deadObjectStats;
	std::vector<deadObjectStats> deadSpace;
};

}
#endif /* BALANCEDCOLLECTOR_HPP_ */
