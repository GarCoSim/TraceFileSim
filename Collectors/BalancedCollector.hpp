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
	std::vector<unsigned int> copyToRegions[MAXREGIONAGE];
	void buildCollectionSet();
	void preCollect();
	void copy();
	void copyRootObjects();
	void copyObject(Object* object, int regionAge);
	void emptyHelpers();
	void copyObjects();
	

};

}
#endif /* BALANCEDCOLLECTOR_HPP_ */

