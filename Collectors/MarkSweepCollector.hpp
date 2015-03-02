/*
 * MarkSweepCollector.hpp
 *
 *  Created on: 2013-09-04
 *      Author: GarCoSim
 */

#ifndef MARKSWEEPCOLLECTOR_HPP_
#define MARKSWEEPCOLLECTOR_HPP_

#include "../Allocators/Allocator.hpp"
#include "Collector.hpp"
#include "../Main/ObjectContainer.hpp"
#include <queue>

using namespace std;

namespace traceFileSimulator {

class MemoryManager;

class MarkSweepCollector : public Collector {
public:
	MarkSweepCollector();
	virtual ~MarkSweepCollector();
	void collect(int reason);
	void checkWatermark();
	void printStats();
	int promotionPhase();

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
};

} 
#endif /* MARKSWEEPCOLLECTOR_HPP_ */
