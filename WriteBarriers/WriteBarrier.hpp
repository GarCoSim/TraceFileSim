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

 #include <map>

namespace traceFileSimulator {

class Collector;

class WriteBarrier {
public:
	WriteBarrier();
	virtual ~WriteBarrier();
	void setEnvironment(Collector* collector);

	virtual void process(Object *oldChild, Object *child) = 0;

protected:
	Collector* myCollector;
};

} 
#endif /* WRITEBARRIER_HPP_ */
