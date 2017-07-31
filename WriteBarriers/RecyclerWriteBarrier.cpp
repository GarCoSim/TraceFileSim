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

/** Sets the new child's colour to black and increases it's reference count.
 * If the old object was not null then this method calls
 * RecyclerWriteBarrier::deleteReference(Object*) on the old child pointer.
 *
 * @param oldChild Previous value of the reference
 * @param child New value of the reference
 */
void RecyclerWriteBarrier::process(Object *oldChild, Object *child) {

	if (child) {
		child->increaseReferenceCount();
		child->setColor(BLACK);
	}

	if (oldChild) {
		deleteReference(oldChild);
	}


}

/** Decrements the reference count of the object.
 * If the object has a count of 0, calls RecyclerWriteBarrier::release(Object*)
 * if not, calls RecyclerWriteBarrier::candidate(Object*)
 *
 * @param obj The object to have a reference deleted
 */
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

/** Sets the object's colour to black and calls
 * RecyclerWriteBarrier::deleteReference(Object*) on all of
 * it's children. If the object is not a candidate for removal,
 * this method will call Collector::freeObject(Object*)
 *
 * @param obj The object to be released
 */
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

/** Marks the object purple and calls Collector::addCandidate(Object*).
 * If object is already purple, does nothing.
 *
 * @param obj The object to be marked.
 */
void RecyclerWriteBarrier::candidate(Object *obj) {
	if (obj->getColor() != PURPLE) {
		obj->setColor(PURPLE);
		myCollector->addCandidate(obj);
	}
}

RecyclerWriteBarrier::~RecyclerWriteBarrier() {
}

}
