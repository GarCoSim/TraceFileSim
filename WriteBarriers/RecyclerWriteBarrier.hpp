/*
 * RecyclerWriteBarrier.hpp
 *
 *  Created on: 2016-04-11
 *      Author: Johannes
 */

#ifndef _RECYCLERWRITEBARRIER_HPP_
#define _RECYCLERWRITEBARRIER_HPP_

#include "WriteBarrier.hpp"

namespace traceFileSimulator {

class RecyclerWriteBarrier : public WriteBarrier {
public:
	RecyclerWriteBarrier();
	virtual ~RecyclerWriteBarrier();
	
private:
	void process(Object *parent, Object *oldChild, Object *child);
};

} 
#endif /* RECYCLERWRITEBARRIER_HPP_ */
