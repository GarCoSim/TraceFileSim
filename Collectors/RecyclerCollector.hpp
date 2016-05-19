/*
 * RecyclerCollector.hpp
 *
 *  Created on: 2016-05-04
 *      Author: Johannes
 */

#ifndef RECYCLERCOLLECTOR_HPP_
#define RECYCLERCOLLECTOR_HPP_

#include "../Allocators/Allocator.hpp"
#include "Collector.hpp"
#include "../Main/ObjectContainer.hpp"
#include <queue>
#include "../Main/MemoryManager.hpp"
#include <stdio.h>
#include <ctime>
#include <set> 
#include "../defines.hpp"

using namespace std;

namespace traceFileSimulator {

class MemoryManager;

class RecyclerCollector : public Collector {
public:
	RecyclerCollector();
	virtual ~RecyclerCollector();
	void collect(int reason);
	void initializeHeap();
	void freeObject(Object *obj);
	void addCandidate(Object *obj);
	bool candidatesNotContainObj(Object *obj);
	bool candidatesContainObj(Object *obj);
	void removeObjectFromCandidates(Object *obj);


private:
	std::set<Object*> myCandidates;

	void preCollect();
	void markCandidates();
	void markGrey(Object *obj);
	void scan(Object *obj);
	void scanBlack(Object *obj);
	void collectCandidates();
	void collectWhite(Object *obj);
};

} 
#endif /* RECYCLERCOLLECTOR_HPP_ */
