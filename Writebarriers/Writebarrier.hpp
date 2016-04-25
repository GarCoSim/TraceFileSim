/*
 * Writebarrier.hpp
 *
 *  Created on: 2016-04-11
 *      Author: Johannes
 */

#ifndef _WRITEBARRIER_HPP_
#define _WRITEBARRIER_HPP_

#include "../Main/Object.hpp"
#include "../Allocators/Allocator.hpp"

namespace traceFileSimulator {

class Writebarrier {
public:
	Writebarrier();
	virtual ~Writebarrier();
	void setEnvironment(Allocator* allocator);

	virtual void process(Object *parent, Object *oldChild, Object *child);


protected:
	Allocator* myAllocator;

};

} 
#endif /* WRITEBARRIER_HPP_ */
