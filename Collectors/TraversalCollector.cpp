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
#include <sstream>
#include <ctime>

extern TRACE_FILE_LINE_SIZE gLineInTrace;
extern FILE* gLogFile;
extern FILE* gDetLog;
extern FILE* balancedLogFile;
extern FILE* gcFile;
extern int hierDepth;
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

	fprintf(stderr, "GC #%zu at %0.3fs", statGcNumber + 1, elapsed_secs);

	traversalDepth.clear();
	amountRootObjects = 0;
	amountOtherObjects = 0;

	preCollect();

	copy();

	swap();

	updatePointers();

	printTraversalDepthStats();

	postCollect();

	stop = clock();
	elapsed_secs = double(stop - start)/CLOCKS_PER_SEC;
	fprintf(stderr, " took %0.3fs\n", elapsed_secs);
}

/** Swaps the two heap spaces.
 * We need to do this step because we are simulating everything by maintaining
 * an object list if we use a real allocator we do not delete the object,
 * we just remove it from the object list it will then by overwritten afterwards.
 * The real allocator just removes it from the object list
 */
void TraversalCollector::swap() {
	size_t heapPosition= 0;
	Object *currentObj;
	RawObject* raw;
	while(heapPosition < myAllocator->getHeapSize()){

		raw = (RawObject *)myAllocator->getNextObjectAddress(heapPosition);

		if(raw!=NULL){
			currentObj = raw->associatedObject;
			if(!currentObj){
				std::stringstream ss;
				ss << "Object does not exist at heap position " << heapPosition << ". The algorithm for stepping through the heap has failed.\n";
				ERRMSG(ss.str().c_str());
				throw 19;
			}

			heapPosition += myAllocator->getSpaceToNextObject(heapPosition);
			heapPosition += currentObj->getHeapSize();

			if(!myAllocator->isInNewSpace(currentObj)){
				if(!currentObj->isForwarded()){
					statFreedObjects++;
					statFreedDuringThisGC++;
				}
				myMemManager->requestDelete(currentObj, myGeneration == GENERATIONS - 1 ? 1 : 0);
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

/** Sets the heap as a half heap split.
 *
 */
void TraversalCollector::initializeHeap() {
	myAllocator->setHalfHeapSize(true); //false for region-based; Tristan
}

/** Clears all data structures used to order operations.
 *
 */
void TraversalCollector::emptyHelpers() {
	while (!myQueue.empty())
		myQueue.pop();
	while (!myStack.empty())
		myStack.pop();
}

/** Moves all objects to the copy space.
 * Calls either TraversalCollector::breadthFirstCopying() or
 * TraversalCollector::depthFirstCopying() depending on which
 * order is set.
 */
void TraversalCollector::copy() {
	//set all objects to dead and not visible first
	initializeMarkPhase();

	emptyHelpers();

	getAllRoots();

	amountRootObjects = traversalDepth.size();

	switch(order) {
		case breadthFirst:
			breadthFirstCopying();
			break;
		case depthFirst:
			depthFirstCopying();
			break;
		case hierarchical:
			hierarchicalCopying();
			break;
		default:
			break;
	}

	amountOtherObjects = traversalDepth.size() - amountRootObjects;
}

/** Puts all root objects in the appropriate helper structure depending on
 * the set order. Also modifies the remset.
 *
 */
void TraversalCollector::getAllRoots() {
	Object* currentObj;
	size_t i, j;
	if (myGeneration == GENERATIONS - 1) {
		//we are performing a glolab GC and can use it to fix possible rem set problems
		//we clear all rem sets and fill them again while performing the marking
		myMemManager->clearRemSets();

		vector<Object*> roots;

		if(order==breadthFirst){
			for (i = 0; i < NUM_THREADS; i++) {
				roots = myObjectContainer->getRoots(i);
				for (j = 0; j < roots.size(); j++) {
					currentObj = roots[j];
					if (currentObj && !currentObj->getVisited()) {
						currentObj->setVisited(true);
						//add to rem set if the root is in a younger generation.
						if (currentObj->getGeneration() < myGeneration) {
							myMemManager->requestRemSetAdd(currentObj);
						}
						myQueue.push(currentObj);
						traversalDepth.insert( std::pair<int,int>(currentObj->getID(), 1) );
					}
				}
			}
		}
		else if(order==depthFirst){
			/* Do not replace these while loops with for loop!
			 * We start at the largest index and decrement down
			 * to 0, but we use size_t so we can't check for negative.
			 * This while loop will do the same thing,
			 * but will never have to allow fo negative values.
			 */
			i = NUM_THREADS;
			while (i > 0) {
				i--;
				roots = myObjectContainer->getRoots(i);
				j = roots.size();
				while(j > 0) {
					j--;
					currentObj = roots[j];
					if (currentObj) {
						//add to rem set if the root is in a younger generation.
						if (currentObj->getGeneration() < myGeneration) {
							myMemManager->requestRemSetAdd(currentObj);
						}
						myStack.push(currentObj);
						traversalDepth.insert( std::pair<int,int>(currentObj->getID(), 1) );
					}
				}
			}
		}
		else if(order==hierarchical){
			for (i = 0; i < NUM_THREADS; i++) {
				roots = myObjectContainer->getRoots(i);
				for (j = 0; j < roots.size(); j++) {
					currentObj = roots[j];
					if (currentObj) {
						//add to rem set if the root is in a younger generation.
						if (currentObj->getGeneration() < myGeneration) {
							myMemManager->requestRemSetAdd(currentObj);
						}
						currentObj->setDepth(0);
						myDoubleQueue.push_back(currentObj);
						traversalDepth.insert( std::pair<int,int>(currentObj->getID(), 1) );
					}
				}
			}
		}
	}
	else {
		for (j = 0; j < myObjectContainer->getGenRootSize(); j++) {
			currentObj = myObjectContainer->getGenRoot(j);
			if (currentObj && !currentObj->getVisited()) {
				currentObj->setVisited(true);
				myQueue.push(currentObj);
				myStack.push(currentObj);
				traversalDepth.insert( std::pair<int,int>(currentObj->getID(), 1) );
			}
		}
	}
}

/** Moves an object to the copy space and adds a forward pointer.
 *
 * @param o Object to be moved.
 */
void TraversalCollector::copyAndForwardObject(Object *o) {
	statCopiedDuringThisGC++;
	statCopiedObjects++;
	void *addressBefore, *addressAfter;
	addressBefore = o->getAddress();
	myAllocator->moveObject(o);
	addressAfter = o->getAddress();
	addForwardingEntry(addressBefore, addressAfter);
}

/** Copies the objects from the queue helper data structure
 * to the copy space.
 */
void TraversalCollector::breadthFirstCopying() {
	size_t i;
	Object* currentObj;
	Object* child;

	//breadth first through the tree using a queue
	while (!myQueue.empty()) {
		currentObj = myQueue.front();
		myQueue.pop();
		size_t kids = currentObj->getPointersMax();
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
				traversalDepth.insert( std::pair<int,int>(child->getID(),  ((traversalDepth.find(currentObj->getID())->second) +1)   ) );
			}
		}
	}
}

/** Copies the objects from the stack helper data structure
 * to the copy space.
 */
void TraversalCollector::depthFirstCopying() {
	size_t i;
	Object* currentObj;
	Object* child;

	//depth first through the tree using a stack
	while (!myStack.empty()) {
		currentObj = myStack.top();
		myStack.pop();
		if(currentObj->getVisited()==false){
			currentObj->setVisited(true);
		}
		else{
			continue;
		}
		size_t kids = currentObj->getPointersMax();

		copyAndForwardObject(currentObj);
		currentObj->setAge(currentObj->getAge() + 1);
		/* Do not replace this while loop with a for loop!
		 * We start at the largest index and decrement down
		 * to 0, but we use size_t so we can't check for negative.
		 * This while loop will do the same thing,
		 * but will never have to allow fo negative values.
		 */
		i = kids;
		while (i >0) {
			i--;
			child = currentObj->getReferenceTo(i);
			//no matter if the child was processed before or not, add it to the rem set.
			if(child && child->getGeneration() < currentObj->getGeneration()){
				myMemManager->requestRemSetAdd(child);
			}
			if (child && !child->getVisited() && child->getGeneration() <= myGeneration) {

				myStack.push(child);
				traversalDepth.insert( std::pair<int,int>(child->getID(),  ((traversalDepth.find(currentObj->getID())->second) +1)   ) );
			}
		}
	}
}

//Access from front only
//Rootset: Place on back in order
//Non-leaf: Place children on front in reverse order
//Leaf: Place children on back in order
void TraversalCollector::hierarchicalCopying() {
	int i;
	Object* currentObj;
	Object* child;

	while (!myDoubleQueue.empty()) {
		currentObj = myDoubleQueue.front();
		myDoubleQueue.pop_front();
		if(currentObj->getVisited()==false){
			currentObj->setVisited(true);
		}
		else{
			continue;
		}
		int kids = currentObj->getPointersMax();
		copyAndForwardObject(currentObj);
		currentObj->setAge(currentObj->getAge() + 1);

		//Leaf of mini-tree: switch to breadth-first
		if(currentObj->getDepth() % hierDepth == hierDepth-1){
			for(i = 0; i < kids; i++){
				child = currentObj->getReferenceTo(i);
				if(child && child->getGeneration() < currentObj->getGeneration()){
					myMemManager->requestRemSetAdd(child);
				}
				if (child && !child->getVisited() && child->getGeneration() <= myGeneration) {
					child->setDepth(currentObj->getDepth() + 1);
					myDoubleQueue.push_back(child);
					traversalDepth.insert( std::pair<int,int>(child->getID(),  ((traversalDepth.find(currentObj->getID())->second) +1)   ) );
				}
			}
		}
		//Non-leaf: depth-first
		else{
			for (i = kids-1; i >=0; i--) {
				child = currentObj->getReferenceTo(i);
				if(child && child->getGeneration() < currentObj->getGeneration()){
					myMemManager->requestRemSetAdd(child);
				}
				if (child && !child->getVisited() && child->getGeneration() <= myGeneration) {
					child->setDepth(currentObj->getDepth() + 1);
					myDoubleQueue.push_front(child);
					traversalDepth.insert( std::pair<int,int>(child->getID(),  ((traversalDepth.find(currentObj->getID())->second) +1)   ) );
				}
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
