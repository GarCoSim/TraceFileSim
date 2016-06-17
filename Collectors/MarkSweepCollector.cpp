/*
 * MarkSweepCollector.cpp
 *
 *  Created on: 2013-09-04
 *      Author: GarCoSim
 */

#include "Collector.hpp"
#include "MarkSweepCollector.hpp"
#include "../Allocators/Allocator.hpp"
#include "../Main/ObjectContainer.hpp"
#include "../Main/MemoryManager.hpp"

extern int gLineInTrace;
extern FILE* gLogFile;
extern FILE* gDetLog;

FILE* gcFile;

clock_t start, stop;

namespace traceFileSimulator {

MarkSweepCollector::MarkSweepCollector() {
}

/**
 * Argument indicates the reason for collection: 0 - unknown, 1 - failed alloc, 2 - high watermark
 */
void MarkSweepCollector::collect(int reason) {
	statCollectionReason = reason;
	stop = clock();
	double elapsed_secs = double(stop - start)/CLOCKS_PER_SEC;
	fprintf(stderr, "GC #%d at %0.3fs", statGcNumber + 1, elapsed_secs);
	preCollect();
	mark();
	sweep();
	/* Uncomment this to enable compaction
	if (statFreedDuringThisGC > 0) {
		compact();
		updatePointers();
	}
	*/

	postCollect();
	
	stop = clock();
	elapsed_secs = double(stop - start)/CLOCKS_PER_SEC;
	fprintf(stderr, " took %0.3fs\n", elapsed_secs);
}

void MarkSweepCollector::initializeHeap() {
	myAllocator->setHalfHeapSize(false);
}

void MarkSweepCollector::mark() {
	initializeMarkPhase();
	enqueueAllRoots();

	//breadth first through the tree
	int i;
	while (!myQueue.empty()) {
		Object* currentObj = myQueue.front();
		myQueue.pop();
		Object* child;
		int kids = currentObj->getPointersMax();
		currentObj->setAge(currentObj->getAge() + 1);
		for (i = 0; i < kids; i++) {
			child = currentObj->getReferenceTo(i);
			//no matter if the child was processed before or not, add it to the rem set.
			if(child && child->getGeneration() < currentObj->getGeneration())
				myMemManager->requestRemSetAdd(child);
			if (child && !child->getVisited() && child->getGeneration() <= myGeneration) {
				child->setVisited(true);
				myQueue.push(child);
			}
		}
	}
}

void MarkSweepCollector::sweep() {
	Object* currentObj;
	int gGC;
	size_t heapPosition= 0;
	RawObject* raw;
	
	while(heapPosition<myAllocator->getHeapSize()){
		
		raw = (RawObject *)myAllocator->getNextObjectAddress(heapPosition);
		
		if(raw!=NULL){
			currentObj = (Object *)raw->associatedObject;
			//currentObj = *(Object **)(objAddress+sizeof(Object *));
			heapPosition += myAllocator->getSpaceToNextObject(heapPosition);
			heapPosition += currentObj->getHeapSize();
			
			if(myGeneration == GENERATIONS -1){
				gGC = 1;
			} else {
				gGC = 0;
			}
			if (currentObj && !currentObj->getVisited()) {
				
				myMemManager->requestDelete(currentObj, gGC);
				statFreedObjects++;
				statFreedDuringThisGC++;
			}
			if(!currentObj){
				break;
			}
		}
		else{
			break;
		}
	}
}


void MarkSweepCollector::enqueueAllRoots() {
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
					myQueue.push(currentObj);
				}
			}
		}
	} else {
		for (j = 0; j < myObjectContainer->getGenRootSize(); j++) {
			currentObj = myObjectContainer->getGenRoot(j);
			if (currentObj && !currentObj->getVisited()) {
				currentObj->setVisited(true);
				myQueue.push(currentObj);
			}
		}
	}
}









MarkSweepCollector::~MarkSweepCollector() {
}

} 
