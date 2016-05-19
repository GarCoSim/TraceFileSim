/*
 * RecyclerWriteBarrier.cpp
 *
 *  Created on: 2016-04-11
 *      Author: Johannes
 *
 */

#include "RecyclerWriteBarrier.hpp"


namespace traceFileSimulator {

RecyclerWriteBarrier::RecyclerWriteBarrier() {

}


void RecyclerWriteBarrier::process(Object *oldChild, Object *child) {

	if (child) {
		child->increaseReferenceCount();
		child->setColor(BLACK);
	}

	if (oldChild) {
		deleteReference(oldChild);
	}


}

void RecyclerWriteBarrier::deleteReference(Object *obj) {
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

void RecyclerWriteBarrier::release(Object *obj) {
	int children = obj->getPointersMax();
	int i;
	Object *child;
	for (i = 0; i < children; i++) {
		child = obj->getReferenceTo(i);
		deleteReference(child);
	}

	obj->setColor(BLACK);
	
	if (myCollector->candidatesNotContainObj(obj)) {
		myCollector->freeObject(obj);
	}
}

void RecyclerWriteBarrier::candidate(Object *obj) {
	if (obj->getColor() != PURPLE) {
		obj->setColor(PURPLE);
		myCollector->addCandidate(obj);
	}
}

RecyclerWriteBarrier::~RecyclerWriteBarrier() {
}

}
