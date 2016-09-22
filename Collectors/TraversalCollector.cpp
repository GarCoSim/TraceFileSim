/*
 * TraversalCollector.cpp
 *
 *  Created on: 2015-02-16
 *      Author: GarCoSim
 */

#include "Collector.hpp"
#include "TraversalCollector.hpp"
#include "../Allocators/Allocator.hpp"
#include "../Main/ObjectContainer.hpp"
#include "../Main/MemoryManager.hpp"

extern int gLineInTrace;
extern FILE* gLogFile;
extern FILE* gDetLog;

extern FILE* gcFile;

extern clock_t start, stop;

namespace traceFileSimulator {
// This collector immplements a split-heap copying collection policy
TraversalCollector::TraversalCollector() {
}

/**
 * Argument indicates the reason for collection: 0 - unknown, 1 - failed alloc, 2 - high watermark
 */
void TraversalCollector::collect(int reason) {
	statCollectionReason = reason;
	stop = clock();
	double elapsed_secs = double(stop - start)/CLOCKS_PER_SEC;
	fprintf(stderr, "GC #%d at %0.3fs", statGcNumber + 1, elapsed_secs);
	preCollect();

	copy();

	swap();

	updatePointers();

	postCollect();

	stop = clock();
	elapsed_secs = double(stop - start)/CLOCKS_PER_SEC;
	fprintf(stderr, " took %0.3fs\n", elapsed_secs);
}

void TraversalCollector::swap() {
	/*
	 * we need to do this step because we are simulating everything by maintaining an object list
	 * if we use a real allocator we do not delete the object, we just remove it from the object list
	 * it will then by overwritten afterwards. the real allocator just removes it from the object list
	 */
	size_t heapPosition= myAllocator->getOldSpaceStartHeapIndex();
	if(heapPosition==5000000){
		int temp = 0;
	}
	Object *currentObj;
	RawObject* raw;
	while(heapPosition<myAllocator->getOldSpaceEndHeapIndex()){
		
		raw = (RawObject *)myAllocator->getNextObjectAddress(heapPosition);
		
		if(raw!=NULL){
			currentObj = (Object *)raw->associatedObject;
			heapPosition += myAllocator->getSpaceToNextObject(heapPosition);
			heapPosition += currentObj->getHeapSize();
			if (currentObj) {
				if(!myAllocator->isInNewSpace(currentObj)){
					if(!currentObj->isForwarded()){
						statFreedObjects++;
						statFreedDuringThisGC++;
					}
					myMemManager->requestDelete(currentObj, myGeneration == GENERATIONS - 1 ? 1 : 0);
					//myObjectContainer->deleteObject(currentObj, !myAllocator->isRealAllocator());
				}
			}
			else{
				break;
			}
		}
		else{
			break;
		}
	}

	// free entire old space
	myAllocator->freeOldSpace();

	myAllocator->swapHeaps();
}

void TraversalCollector::initializeHeap() {
	myAllocator->setHalfHeapSize(true); //false for region-based; Tristan
}

void TraversalCollector::emptyHelpers() {
	while (!myQueue.empty())
		myQueue.pop();
	while (!myStack.empty())
		myStack.pop();
}

void TraversalCollector::copy() {
	//set all objects to dead and not visible firs
	initializeMarkPhase();

	emptyHelpers();

	getAllRoots();

	switch(order) {
		case breadthFirst:
			breadthFirstCopying();
			break;
		case depthFirst:
			depthFirstCopying();
			break;
		default:
			break;
	}
}

void TraversalCollector::getAllRoots() {
	Object* currentObj;
	int i, j;
	if (myGeneration == GENERATIONS - 1) {
		//we are performing a glolab GC and can use it to fix possible rem set problems
		//we clear all rem sets and fill them again while performing the marking
		myMemManager->clearRemSets();

		vector<Object*> roots;
		for (i = 0; i < NUM_THREADS; i++) {
			roots = myObjectContainer->getRoots(i);
			for (j = 0; j < (int)roots.size(); j++) {
				currentObj = roots[j];
				if (currentObj && !currentObj->getVisited()) {
					currentObj->setVisited(true);
					//add to rem set if the root is in a younger generation.
					if (currentObj->getGeneration() < myGeneration) {
						myMemManager->requestRemSetAdd(currentObj);
					}
					switch(order) {
						case breadthFirst:
							myQueue.push(currentObj);
							break;
						case depthFirst:
							myStack.push(currentObj);
							break;
						default:
						break;
					}
				}
			}
		}
	} else {
		for (j = 0; j < myObjectContainer->getGenRootSize(); j++) {
			currentObj = myObjectContainer->getGenRoot(j);
			if (currentObj && !currentObj->getVisited()) {
				currentObj->setVisited(true);
				myQueue.push(currentObj);
				myStack.push(currentObj);
			}
		}
	}
}

void TraversalCollector::copyAndForwardObject(Object *o) {
	void *addressBefore, *addressAfter;
	addressBefore = (void *) o->getAddress();
	myAllocator->moveObject(o);
	addressAfter = (void *) o->getAddress();
	addForwardingEntry(addressBefore, addressAfter);
}

void TraversalCollector::breadthFirstCopying() {
	int i;
	Object* currentObj;
	Object* child;

	//breadth first through the tree using a queue
	while (!myQueue.empty()) {
		currentObj = myQueue.front();
		myQueue.pop();
		int kids = currentObj->getPointersMax();
		copyAndForwardObject(currentObj);
		currentObj->setAge(currentObj->getAge() + 1);
		for (i = 0; i < kids; i++) {
			child = currentObj->getReferenceTo(i);
			//no matter if the child was processed before or not, add it to the rem set.
			if(child && child->getGeneration() < currentObj->getGeneration()){
				myMemManager->requestRemSetAdd(child);
			}
			if (child && !child->getVisited() && child->getGeneration() <= myGeneration) {
				child->setVisited(true);

				myQueue.push(child);
			}
		}
	}
}

void TraversalCollector::depthFirstCopying() {
	int i;
	Object* currentObj;
	Object* child;

	//depth first through the tree using a stack
	while (!myStack.empty()) {
		currentObj = myStack.top();
		myStack.pop();
		int kids = currentObj->getPointersMax();
		copyAndForwardObject(currentObj);
		currentObj->setAge(currentObj->getAge() + 1);
		for (i = 0; i < kids; i++) {
			child = currentObj->getReferenceTo(i);
			//no matter if the child was processed before or not, add it to the rem set.
			if(child && child->getGeneration() < currentObj->getGeneration()){
				myMemManager->requestRemSetAdd(child);
			}
			if (child && !child->getVisited() && child->getGeneration() <= myGeneration) {
				child->setVisited(true);

				myStack.push(child);
			}
		}
	}
}



void TraversalCollector::reallocateAllLiveObjects() {
	int i;
	vector<Object*> objects = myObjectContainer->getLiveObjects();
	for (i = 0; i < (int)objects.size(); i++) {
		Object* currentObj = objects[i];
		if (currentObj) {
			myMemManager->requestReallocate(currentObj);
		}
	}

}

TraversalCollector::~TraversalCollector() {
}

} 
