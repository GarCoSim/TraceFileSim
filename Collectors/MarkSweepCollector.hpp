/*
 * MarkSweepCollector.hpp
 *
 *  Created on: 2013-09-04
 *      Author: GarCoSim
 */

#ifndef MARKSWEEPCOLLECTOR_HPP_
#define MARKSWEEPCOLLECTOR_HPP_




using namespace std;

namespace traceFileSimulator {

class MemoryManager;

/** A Mark and Sweep Implementation of Collector
 *
 */
class MarkSweepCollector : public Collector {
public:
	MarkSweepCollector();
	virtual ~MarkSweepCollector();
	void collect(int reason);
	void initializeHeap();

private:
	void mark();
	void sweep();
	void enqueueAllRoots();

	
};

} 
#endif /* MARKSWEEPCOLLECTOR_HPP_ */
