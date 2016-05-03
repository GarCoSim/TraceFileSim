/*
 * ZombieRecyclerWriteBarrier.cpp
 *
 *  Created on: 2016-04-11
 *      Author: Johannes
 *
 */

#include "ZombieRecyclerWriteBarrier.hpp"


namespace traceFileSimulator {

ZombieRecyclerWriteBarrier::ZombieRecyclerWriteBarrier() {
}


void ZombieRecyclerWriteBarrier::process(Object *parent, Object *oldChild, Object *child) {

	if (child) {
		child->increaseReferenceCount();
		child->setColor(BLACK);
	}

	if (oldChild) {
		deleteReference(oldChild);
	}
}

void ZombieRecyclerWriteBarrier::deleteReference(Object *obj) {
	if (obj) {
		obj->decreaseReferenceCount();
		if (obj->getReferenceCount() == 0) {
			release(obj);
		}
		else {
			candidate(obj);
		}
	}
}

void ZombieRecyclerWriteBarrier::release(Object *obj) {
	int children = obj->getPointersMax();
	int i;
	Object *child;
	for (i = 0; i < children; i++) {
		child = obj->getReferenceTo(i);
		deleteReference(child);
	}
	obj->setColor(BLACK);
	if (candidates.find(obj->getID()) == candidates.end()) {
		free(obj);
	}
}

void ZombieRecyclerWriteBarrier::free(Object *obj) {
	//fprintf(stderr, "Freeing object %i from ZombieRecycler\n", obj->getID());
	myMemoryManager->requestDelete(obj, 0);
}

void ZombieRecyclerWriteBarrier::candidate(Object *obj) {
	if (obj->getColor() != PURPLE) {
		obj->setColor(PURPLE);
		int id = obj->getID();
		candidates[id] = obj;
	}
}


ZombieRecyclerWriteBarrier::~ZombieRecyclerWriteBarrier() {
}

}
