/*
 * TraversalCollector.hpp
 *
 *  Created on: 2013-09-04
 *      Author: GarCoSim
 */

#ifndef TRAVERSALCOLLECTOR_HPP_
#define TRAVERSALCOLLECTOR_HPP_

#include "Collector.hpp"
#include "../Allocators/Allocator.hpp"
#include "../Main/ObjectContainer.hpp"
#include <queue>
#include <stack>
#include "../defines.hpp"
#include "../Main/MemoryManager.hpp"
#include <stdio.h>
#include <ctime>

using namespace std;

namespace traceFileSimulator {

class MemoryManager;

class TraversalCollector : public Collector {
public:
	TraversalCollector();
	virtual ~TraversalCollector();
	void collect(int reason);
	void checkWatermark();
	int promotionPhase();

private:
	void copy();
	void compact();
	void initializeMarkPhase();
	void preCollect();
	void freeAllLiveObjects();
	void reallocateAllLiveObjects();
	void breadthFirstCopying();
	void depthFirstCopying();
	void hotnessCopying();
	void getAllRoots();
	void emptyHelpers();
	void swap();
};

} 
#endif /* TRAVERSALCOLLECTOR_HPP_ */
