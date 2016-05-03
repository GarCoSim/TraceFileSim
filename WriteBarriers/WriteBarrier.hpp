/*
 * WriteBarrier.hpp
 *
 *  Created on: 2016-04-11
 *      Author: Johannes
 */

#ifndef _WRITEBARRIER_HPP_
#define _WRITEBARRIER_HPP_

#include "../defines.hpp"
#include "../Main/Object.hpp"
//#include "../Main/MemoryManager.hpp"

namespace traceFileSimulator {

class MemoryManager;

class WriteBarrier {
public:
	WriteBarrier();
	virtual ~WriteBarrier();
	void setEnvironment(MemoryManager* memoryManager);

	virtual void process(Object *parent, Object *oldChild, Object *child);


protected:
	MemoryManager* myMemoryManager;

};

} 
#endif /* WRITEBARRIER_HPP_ */
