/*
 * TraversalCollector.hpp
 *
 *  Created on: 2013-09-04
 *      Author: GarCoSim
 */

#ifndef TRAVERSALCOLLECTOR_HPP_
#define TRAVERSALCOLLECTOR_HPP_


using namespace std;

namespace traceFileSimulator {

class MemoryManager;

// This collector immplements a split-heap copying collection policy
class TraversalCollector : public Collector {
public:
	TraversalCollector();
	virtual ~TraversalCollector();
	void collect(int reason);
	void initializeHeap();

private:
	void copy();
	void breadthFirstCopying();
	void depthFirstCopying();
	void getAllRoots();
	void emptyHelpers();
	void swap();
	void copyAndForwardObject(Object *o);
	void reallocateAllLiveObjects();
};

} 
#endif /* TRAVERSALCOLLECTOR_HPP_ */
