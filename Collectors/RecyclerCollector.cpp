/*
 * RecyclerCollector.cpp
 *
 *  Created on: 2016-05-04
 *      Author: Johannes
 */

#include "RecyclerCollector.hpp"

extern int gLineInTrace;
extern FILE* gLogFile;
extern FILE* gDetLog;

extern FILE* gcFile;

extern clock_t start, stop;

namespace traceFileSimulator {

RecyclerCollector::RecyclerCollector() {

}

/**
 * Argument indicates the reason for collection: 0 - unknown, 1 - failed alloc, 2 - high watermark
 */
void RecyclerCollector::collect(int reason) {
	statCollectionReason = reason;

	std::set<Object*>::iterator it = myCandidates.begin();

	preCollect();
	markCandidates();

	it = myCandidates.begin();
 	while (it != myCandidates.end()) {
 		scan(*it);
 		++it;
 	}

	collectCandidates();

	//postCollect();
}

void RecyclerCollector::initializeHeap() {
	myAllocator->setHalfHeapSize(false);
}

void RecyclerCollector::preCollect() {
	start = clock();
	statFreedDuringThisGC = 0;
	statGcNumber++;
}

void RecyclerCollector::addCandidate(Object *obj) {
	myCandidates.insert(obj);
}

bool RecyclerCollector::candidatesNotContainObj(Object *obj) {
	if (myCandidates.find(obj) == myCandidates.end())
		return true;
	else
		return false;
}

bool RecyclerCollector::candidatesContainObj(Object *obj) {
	if (myCandidates.find(obj) == myCandidates.end())
		return false;
	else
		return true;
}

void RecyclerCollector::removeObjectFromCandidates(Object *obj) {
	myCandidates.erase(obj);
}

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

void RecyclerCollector::markGrey(Object *obj) {
	if (obj->getColor() != GREY) {
		obj->setColor(GREY);
		int children = obj->getPointersMax();
		int i;
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

void RecyclerCollector::scan(Object *obj) {
	if (obj->getColor() == GREY) {
		if (obj->getReferenceCount() > 0) {
			scanBlack(obj);
		}
		else {
			obj->setColor(WHITE);
			int children = obj->getPointersMax();
			int i;
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

void RecyclerCollector::scanBlack(Object *obj) {
	obj->setColor(BLACK);
	int children = obj->getPointersMax();
	int i;
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

void RecyclerCollector::collectCandidates() {
	std::set<Object*>::iterator it = myCandidates.begin();
 	while (it != myCandidates.end()) {
 		myCandidates.erase(it);
		collectWhite(*it);
		it++;
 	}
}

void RecyclerCollector::collectWhite(Object *obj) {
	if (obj->getColor() == WHITE) {
		std::set<Object*>::iterator it = myCandidates.find(obj);
		if (it == myCandidates.end()) {
			obj->setColor(BLACK);
			int children = obj->getPointersMax();
			int i;
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

void RecyclerCollector::freeObject(Object *obj) {
	if (obj) {
		myMemManager->requestDelete(obj, 0);
		statFreedObjects++;
		statFreedDuringThisGC++;
	}
}

RecyclerCollector::~RecyclerCollector() {
}

} 
