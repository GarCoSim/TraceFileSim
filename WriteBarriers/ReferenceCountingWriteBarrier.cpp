/*
 * ReferenceCountingWriteBarrier.cpp
 *
 *  Created on: 2016-05-06
 *      Author: Johannes
 *
 */

#include "ReferenceCountingWriteBarrier.hpp"


namespace traceFileSimulator {

ReferenceCountingWriteBarrier::ReferenceCountingWriteBarrier() {

}


void ReferenceCountingWriteBarrier::process(Object *oldChild, Object *child) {

	if (child) {
		child->increaseReferenceCount();
	}

	if (oldChild) {
		deleteReference(oldChild);
	}
}

void ReferenceCountingWriteBarrier::deleteReference(Object *obj) {
	if (obj) {
		obj->decreaseReferenceCount();

		if (obj->getReferenceCount() == 0) {

			int children = obj->getPointersMax();
			int i;
			Object *child;
			for (i = 0; i < children; i++) {
				child = obj->getReferenceTo(i);
				deleteReference(child);
			}

			myCollector->freeObject(obj);
		}
	}
}


ReferenceCountingWriteBarrier::~ReferenceCountingWriteBarrier() {
}

}
