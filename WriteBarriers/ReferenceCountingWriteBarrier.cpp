/*
 * ReferenceCountingWriteBarrier.cpp
 *
 *  Created on: 2016-05-06
 *      Author: Johannes
 *
 */

#include "ReferenceCountingWriteBarrier.hpp"

extern int lockNumber;

namespace traceFileSimulator {

ReferenceCountingWriteBarrier::ReferenceCountingWriteBarrier() {

}

/** Increases the reference count of the new object reference and calls
 * ReferenceCountingWriteBarrier::deleteReference(Object*) on the
 * old child object reference.
 *
 * @param oldChild Previous value of the reference.
 * @param child New value of the reference.
 */
void ReferenceCountingWriteBarrier::process(Object *oldChild, Object *child) {

	if (child) {
		child->increaseReferenceCount();
	}

	if (oldChild) {
		deleteReference(oldChild);
	}
}

/** Decreases the reference count of the object. If the count becomes 0
 * due to this decrement then the method will recursively call itself on
 * all children of the supplied object and then call
 * Collector::freeObject(Object*)
 *
 * @param obj Object to have a reference deleted
 */
void ReferenceCountingWriteBarrier::deleteReference(Object *obj) {
	if (obj) {
		obj->decreaseReferenceCount();

		if (obj->getReferenceCount() == 0 && (lockNumber == 0) ) {

			int children = obj->getPointersMax();
			int i;
			Object *child;
			for (i = 0; i < children; i++) {
				child = obj->getReferenceTo(i);
				deleteReference(child);
			}

			myCollector->freeObject(obj);
		}
		else if ( (obj->getReferenceCount()) == 0 ) { //Reference count is zero but trace file locked.
			myCollector->addDeadObjectLocked(obj);
		}
	}
}


void ReferenceCountingWriteBarrier::alreadyDeadObject(Object *obj) {
	if (obj) {
		if ( (obj->getReferenceCount()) == 0 && (lockNumber == 0) ) {

			int children = obj->getPointersMax();
			int i;
			Object *child;
			for (i = 0; i < children; i++) {
				child = obj->getReferenceTo(i);
				deleteReference(child);
			}

			myCollector->freeObject(obj);
		}
		else if ( (obj->getReferenceCount()) == 0 ) { //Reference count is zero but trace file locked.
			myCollector->addDeadObjectLocked(obj);
		}
	}
}


ReferenceCountingWriteBarrier::~ReferenceCountingWriteBarrier() {
}

}
