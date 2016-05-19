/*
 * BasicAllocator.hpp
 *
 *  Created on: 2015-09-04
 *      Author: GarCoSim
 *
 * This collector implements a simple first-fit allocation policy.
 */

#ifndef _BASICALLOCATOR_HPP_
#define _BASICALLOCATOR_HPP_

#include "Allocator.hpp"
#include "../Main/Object.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <climits>
#include "../defines.hpp"
#include <memory.h>
#include <string>
#include <sys/time.h> 
 
namespace traceFileSimulator {

class BasicAllocator : public Allocator {
public:
	BasicAllocator();
	virtual ~BasicAllocator();

	bool isRealAllocator();

private:
	void *allocate(size_t size, size_t lower, size_t upper);


	//unsigned char *heap; //now in Allocator.hpp
};

} 
#endif /* _BASICALLOCATOR_HPP_ */
