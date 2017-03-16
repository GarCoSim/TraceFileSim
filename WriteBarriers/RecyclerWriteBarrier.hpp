/*
 * RecyclerWriteBarrier.hpp
 *
 *  Created on: 2016-04-11
 *      Author: Johannes
 */

#ifndef _RECYCLERWRITEBARRIER_HPP_
#define _RECYCLERWRITEBARRIER_HPP_

#include "WriteBarrier.hpp"
#include "../Main/Object.hpp"
#include "../Collectors/Collector.hpp"


namespace traceFileSimulator {

class Collector;

/** A recycler implementation of the WriteBarrier class
 *
 */
class RecyclerWriteBarrier : public WriteBarrier {
public:
	RecyclerWriteBarrier();
	virtual ~RecyclerWriteBarrier();
	
private:
	void process(Object *oldChild, Object *child);
	void deleteReference(Object *obj);
	void release(Object *obj);
	void candidate(Object *obj);
	void alreadyDeadObject(Object *obj);
};

} 
#endif /* RECYCLERWRITEBARRIER_HPP_ */
