/*
 * ZombieRecyclerWriteBarrier.hpp
 *
 *  Created on: 2016-04-11
 *      Author: Johannes
 */

#ifndef _ZOMBIERECYCLERWRITEBARRIER_HPP_
#define _ZOMBIERECYCLERWRITEBARRIER_HPP_

#include "WriteBarrier.hpp"
#include "../Main/Object.hpp"
#include "../Main/MemoryManager.hpp"

#include <map>

namespace traceFileSimulator {

class MemoryManager;

class ZombieRecyclerWriteBarrier : public WriteBarrier {
public:
	ZombieRecyclerWriteBarrier();
	virtual ~ZombieRecyclerWriteBarrier();
	
private:
	void process(Object *parent, Object *oldChild, Object *child);
	void deleteReference(Object *obj);
	void release(Object *obj);
	void free(Object *obj);
	void candidate(Object *obj);

	std::map<int, Object*> candidates;
};

} 
#endif /* ZOMBIERECYCLERWRITEBARRIER_HPP_ */
