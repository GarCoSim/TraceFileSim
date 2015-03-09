/*
 * RealAllocator.hpp
 *
 *  Created on: 2013-09-03
 *      Author: GarCoSim
 */

#ifndef _REALALLOCATOR_HPP_
#define _REALALLOCATOR_HPP_

#include "Allocator.hpp"
#include "../Main/Object.hpp"
#include <stdio.h>

namespace traceFileSimulator {

class RealAllocator : public Allocator {
public:
	RealAllocator();
	virtual ~RealAllocator();

	bool isRealAllocator();
	void freeAllSectors();
	void gcFree(Object* object);

private:
	size_t allocate(int size, int lower, int upper, int lastAddress);

	unsigned char *heap;
};

} 
#endif /* _REALALLOCATOR_HPP_ */
