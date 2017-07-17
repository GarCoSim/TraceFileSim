/*
 * ReferenceCountingWriteBarrier.hpp
 *
 *  Created on: 2016-05-06
 *      Author: Johannes
 */

#ifndef _REFERENCECOUNTINGWRITEBARRIER_HPP_
#define _REFERENCECOUNTINGWRITEBARRIER_HPP_

#include "WriteBarrier.hpp"
#include "../Main/Object.hpp"
#include "../Collectors/Collector.hpp"


namespace traceFileSimulator {

class Collector;

/** Standard implementation of the reference counting write barrier.
 * Implements the WriteBarrier class.
 *
 */
class ReferenceCountingWriteBarrier : public WriteBarrier {
public:
	ReferenceCountingWriteBarrier();
	virtual ~ReferenceCountingWriteBarrier();
	
private:
	void process(Object *oldChild, Object *child);
	void deleteReference(Object *obj);
};

} 
#endif /* REFERENCECOUNTINGWRITEBARRIER_HPP_ */
