/*
 * NextFitAllocator.hpp
 *
 *  Created on: 2015-09-04
 *      Author: GarCoSim
 *
 * This collector implements a next-fit allocation policy.
 */

#ifndef _NEXTFITALLOCATOR_HPP_
#define _NEXTFITALLOCATOR_HPP_

#include "Allocator.hpp"
 
namespace traceFileSimulator {

class NextFitAllocator : public Allocator {
public:
	NextFitAllocator();
	virtual ~NextFitAllocator();

	bool isRealAllocator();

private:
	void *allocate(size_t size, size_t lower, size_t upper);

	//unsigned char *heap; //now in Allocator.hpp
};

} 
#endif /* _NEXTFITALLOCATOR_HPP_ */
