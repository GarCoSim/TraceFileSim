/*
 * RealAllocator.hpp
 *
 *  Created on: 2013-09-03
 *      Author: GarCoSim
 */

#ifndef _REALALLOCATOR_HPP_
#define _REALALLOCATOR_HPP_

#include "Allocator.hpp"
 
namespace traceFileSimulator {
	
class Object;

class RealAllocator : public Allocator {
public:
	RealAllocator();
	virtual ~RealAllocator();

	bool isRealAllocator();
	void gcFree(Object* object);
	void initializeHeap(size_t heapSize);
	void freeOldSpace();

private:
	void *allocate(size_t size, size_t lower, size_t upper);

	//unsigned char *heap; //now in Allocator.hpp
};

} 
#endif /* _REALALLOCATOR_HPP_ */
