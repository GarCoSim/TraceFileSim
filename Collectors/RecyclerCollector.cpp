/*
 * RecyclerCollector.cpp
 *
 *  Created on: 2016-05-04
 *      Author: Johannes
 */

#include "Collector.hpp"
#include "RecyclerCollector.hpp"
#include "../Allocators/Allocator.hpp"
#include "../Main/Object.hpp"
#include "../Main/MemoryManager.hpp"

extern LINESIZE gLineInTrace;
extern FILE* gLogFile;
extern FILE* gDetLog;

extern FILE* gcFile;

extern clock_t start, stop;

namespace traceFileSimulator {

RecyclerCollector::RecyclerCollector() {
}

/** Argument indicates the reason for collection: 0 - unknown, 1 - failed alloc, 2 - high watermark
 *
 */
void RecyclerCollector::collect(int reason) {
	statCollectionReason = reason;

	std::set<Object*>::iterator it = myCandidates.begin();

	preCollect();
	markCandidates();

 	while (it != myCandidates.end()) {
 		scan(*it);
 		++it;
 	}

	collectCandidates();

	postCollect();
}

/** Ensures heap is in the correct state to use this type of collector
 *
 */
void RecyclerCollector::initializeHeap() {
	myAllocator->setHalfHeapSize(false);
}

/** Inserts the object reference into a data structure containing
 * all objects that are candidates for removal
 *
 * @param obj reference to the object to be inserted to the data structure
 */
void RecyclerCollector::addCandidate(Object *obj) {
	myCandidates.insert(obj);
}

/** Checks if the object is in the candidates data structure
 *
 * @param obj reference to object to be checked
 * @return true if the object is NOT in the data structure, false otherwise
 */
bool RecyclerCollector::candidatesNotContainObj(Object *obj) {
	return (myCandidates.find(obj) == myCandidates.end());
}

/** Marks all purple objects in the candidates data structure grey
 * and frees all black objects in the candidates data structure.
 */
void RecyclerCollector::markCandidates() {
	std::set<Object*>::iterator it = myCandidates.begin();
	Object *currentObj;

 	while (it != myCandidates.end()) {
 		currentObj = *it;
 		if (currentObj->getColor() == PURPLE) {
 			markGrey(currentObj);
 			++it;
 		}
 		else {
 			if (currentObj->getColor() == BLACK && currentObj->getReferenceCount() == 0) {
 				freeObject(currentObj);
 			}
 			myCandidates.erase(it++);
 		}
 	}
}

/** Sets the colour of the object and all it's children to grey.
 *
 * @param obj Object at the root of an object sub-graph
 */
void RecyclerCollector::markGrey(Object *obj) {
	if (obj->getColor() != GREY) {
		obj->setColor(GREY);
		size_t children = obj->getPointersMax();
		size_t i;
		Object *child;
		for (i = 0; i < children; i++) {
			child = obj->getReferenceTo(i);
			if (child) {
				child->decreaseReferenceCount();
				markGrey(child);
			}
		}
	}
}

/** Changes grey objects to either black or white as appropriate.
 * If setting the object white, then scan all children.
 * Does not change non-grey objects
 *
 * @param obj An object to be scanned.
 */
void RecyclerCollector::scan(Object *obj) {
	if (obj->getColor() == GREY) {
		if (obj->getReferenceCount() > 0) {
			scanBlack(obj);
		}
		else {
			obj->setColor(WHITE);
			size_t children = obj->getPointersMax();
			size_t i;
			Object *child;
			for (i = 0; i < children; i++) {
				child = obj->getReferenceTo(i);
				if (child) {
					scan(child);
				}
			}
		}
	}
}

/** Sets an object and all it's children black.
 *
 * @param obj Object to be set black
 */
void RecyclerCollector::scanBlack(Object *obj) {
	obj->setColor(BLACK);
	size_t children = obj->getPointersMax();
	size_t i;
	Object *child;
	for (i = 0; i < children; i++) {
		child = obj->getReferenceTo(i);
		if (child) {
			child->increaseReferenceCount();
			if (child->getColor() != BLACK) {
				scanBlack(child);
			}
		}
	}
}

/** Clears the candidates data structure, freeing all white objects
 * in the data structure.
 */
void RecyclerCollector::collectCandidates() {
	std::set<Object*>::iterator it = myCandidates.begin();
 	while (it != myCandidates.end()) {
		Object* candidate = *it;
 		myCandidates.erase(it++);
		collectWhite(candidate);
 	}
}

/** If the object is white, marks it black, then recursively calls itself
 * on all the object's children. After the children have been processed,
 * the object is collected.
 *
 * @param obj Object to be processed
 */
void RecyclerCollector::collectWhite(Object *obj) {
	if (obj->getColor() == WHITE) {
		std::set<Object*>::iterator it = myCandidates.find(obj);
		if (it == myCandidates.end()) {
			obj->setColor(BLACK);
			size_t children = obj->getPointersMax();
			size_t i;
			Object *child;
			for (i = 0; i < children; i++) {
				child = obj->getReferenceTo(i);
				if (child) {
					collectWhite(child);
				}
			}
			freeObject(obj);
		}
	}
}


RecyclerCollector::~RecyclerCollector() {
}

} 
