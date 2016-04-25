/*
 * Recycler.hpp
 *
 *  Created on: 2016-04-11
 *      Author: Johannes
 */

#ifndef _RECYCLER_HPP_
#define _RECYCLER_HPP_

#include "Writebarrier.hpp"

namespace traceFileSimulator {

class Recycler : public Writebarrier {
public:
	Recycler();
	virtual ~Recycler();
	
private:
	void process(Object *parent, Object *oldChild, Object *child);
};

} 
#endif /* RECYCLER_HPP_ */
