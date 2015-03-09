/*
 * SimulatedAllocator.hpp
 *
 *  Created on: 2013-09-03
 *      Author: GarCoSim
 */

#ifndef _SIMULATEDALLOCATOR_HPP_
#define _SIMULATEDALLOCATOR_HPP_

#include "Allocator.hpp"
#include "../Main/Object.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <climits>
#include "../defines.hpp"
#include <string>

namespace traceFileSimulator {

class SimulatedAllocator : public Allocator {
public:
	SimulatedAllocator();
	virtual ~SimulatedAllocator();

	bool isRealAllocator();
	void freeAllSectors();
	void gcFree(Object* object);
	void initializeHeap(int heapSize);
	
private:
	size_t allocate(int size, int lower, int upper, size_t lastAddress);
};

} 
#endif /* SIMULATEDALLOCATOR_HPP_ */
