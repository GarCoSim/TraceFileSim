/*
 * RecyclerCollector.hpp
 *
 *  Created on: 2016-05-04
 *      Author: Johannes
 */

#ifndef RECYCLERCOLLECTOR_HPP_
#define RECYCLERCOLLECTOR_HPP_


#include <set> 

using namespace std;

namespace traceFileSimulator {

class MemoryManager;

/** Implements the Recycler Collector used to supplement reference counting
 * to ensure correctness of algorithm.
 *
 */
class RecyclerCollector : public Collector {
public:
	RecyclerCollector();
	virtual ~RecyclerCollector();
	void collect(int reason);
	void initializeHeap();
	void addCandidate(Object *obj);
	bool candidatesNotContainObj(Object *obj);


private:
	std::set<Object*> myCandidates;

	void markCandidates();
	void markGrey(Object *obj);
	void scan(Object *obj);
	void scanBlack(Object *obj);
	void collectCandidates();
	void collectWhite(Object *obj);
};

} 
#endif /* RECYCLERCOLLECTOR_HPP_ */
