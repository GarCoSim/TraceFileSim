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

class BalancedCollector : public Collector {
public:
	BalancedCollector();
	virtual ~BalancedCollector();
	void collect(int reason);
	void initializeHeap();

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
	void copyObject(Object* object, int regionAge);
	int copyAndForwardObject(Object *obj);
	void updatePointers();
	void emptyHelpers();
	void removeObjects();
	void updateRemsetPointers();
	void printObjects(); //Good function to get stats from runs with Aaaron's Test Tracefiles
	void printObjectInfo(Object *obj);
	void reOrganizeRegions();
	void printFinalStats();

	int totalObjectsInCollectionSet;

	typedef struct deadObjectStats{
		unsigned int regionID;
		size_t percentDead; //percentage of the region occupied by dead objects
	} deadObjectStats;
	std::vector<deadObjectStats> deadSpace;
};

}
#endif /* BALANCEDCOLLECTOR_HPP_ */
