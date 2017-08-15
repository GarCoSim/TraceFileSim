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
#include <stdio.h>

 #include <map>

namespace traceFileSimulator {

class Collector;

/** A class representing the write barrier required to do reference counting
 * safely in a concurrent environment.
 *
 * @author Johannes
 */
class WriteBarrier {
public:
	WriteBarrier();
	virtual ~WriteBarrier();
	void setEnvironment(Collector* collector);

	virtual void process(Object *oldChild, Object *child) = 0;
	virtual void alreadyDeadObject(Object *obj) = 0;

protected:
	Collector* myCollector;
};

} 
#endif /* WRITEBARRIER_HPP_ */
