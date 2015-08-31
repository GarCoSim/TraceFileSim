/*
 * CopyingCollector.cpp
 *
 *  Created on: 2013-09-04
 *      Author: GarCoSim
 */

#include "CopyingCollector.hpp"

extern int gLineInTrace;
extern FILE* gLogFile;

namespace traceFileSimulator {

CopyingCollector::CopyingCollector() {
}

/**
 * Argument indicates the reason for collection: 0 - unknown, 1 - failed alloc, 2 - high watermark
 */
void CopyingCollector::collect(int reason){
	statCollectionReason = reason;
	preCollect();
	//easy as 1, 2, 3 :)
	mark();
	sweep();
	//comment in order to have mark/sweep only
	if(statFreedDuringThisGC > 0){
		//only compact if you actually freed something
		compact();
	}

	//toggle heap side
	statHeapSide = (statHeapSide+1)%2;

	postCollect();
}

//void CopyingCollector::checkWatermark(){
//	int size = myAllocator->getHeapSize();
//	int free = myAllocator->getFreeSize();
//	int ratio = 100 - (100*free/size);
//	if(ratio > myWatermark){
//		collect(reasonHighWatermark);
//	}
//}

void CopyingCollector::mark(){
	int i;
	//set all objects to dead and not visible firs
	initializeMarkPhase();

	enqueueAllRoots();

	//breadth first through the tree
	while(!myQueue.empty()){
		Object* currentObj = myQueue.front();
		myQueue.pop();
		Object* child;
		int kids = currentObj->getPointersMax();
		currentObj->setVisited(true);
		for(i = 0 ; i < kids ; i++){
			child = currentObj->getReferenceTo(i);
			if(child && !child->getVisited()){
				child->setVisited(true);
				myQueue.push(child);
			}
		}
	}

}

void CopyingCollector::sweep(){
	Object* currentObj;
	int i;
	vector<Object*> objects = myObjectContainer->getLiveObjects();
	for(i = 0 ; i < (int)objects.size(); i++){
		currentObj = objects[i];
		if(currentObj && !currentObj->getVisited()){
			myAllocator->gcFree(currentObj);
			myObjectContainer->deleteObject(currentObj, false);
			statFreedObjects++;
			statFreedDuringThisGC++;
		}
	}
}

void CopyingCollector::enqueueAllRoots(){
	Object* currentObj;
	int i, j;

	vector<Object*> roots;
	for(i = 0 ; i < NUM_THREADS ; i++){
		roots = myObjectContainer->getRoots(i);
		for(j = 0 ; j < (int)roots.size(); j++){
			currentObj = roots[j];
			if(currentObj && !currentObj->getVisited()){
				currentObj->setVisited(true);
				myQueue.push(currentObj);
			}
		}
	}
}

void CopyingCollector::initializeMarkPhase(){
	Object* currentObj;
	int i;
	vector<Object*> objects = myObjectContainer->getLiveObjects();
	for(i = 0 ; i < (int)objects.size(); i++){
		currentObj = objects[i];
		if(currentObj != NULL){
			currentObj->setVisited(false);
		}
	}
}

void CopyingCollector::preCollect(){
	statFreedDuringThisGC = 0;
	statGcNumber++;
}

void CopyingCollector::compact() {
	/*only alive objects are left in the container. If I traverse
	through the live list, I get all elements*/
	//free everything.
	freeAllLiveObjects();
	//allocate everything back.
	myAllocator->setAllocationSearchStart(0);
	reallocateAllLiveObjects();
}

void CopyingCollector::freeAllLiveObjects() {
	int i;
	//set all objects to dead and not visible firs
	initializeMarkPhase();

	enqueueAllRoots();

	//breadth first through the tree
	while(!myQueue.empty()){
		Object* currentObj = myQueue.front();
		myQueue.pop();

		//the actual work is just one line
		myAllocator->gcFree(currentObj);

		Object* child;
		int kids = currentObj->getPointersMax();
		currentObj->setVisited(true);
		for(i = 0 ; i < kids ; i++){
			child = currentObj->getReferenceTo(i);
			if(child && !child->getVisited()){
				child->setVisited(true);
				myQueue.push(child);
			}
		}
	}
}

void CopyingCollector::reallocateAllLiveObjects() {
	int i;
	//set all objects to dead and not visible firs
	initializeMarkPhase();

	enqueueAllRoots();

	//breadth first through the tree
	while(!myQueue.empty()){
		Object* currentObj = myQueue.front();
		myQueue.pop();

		//the actual work
		int size = currentObj->getPayloadSize();
		int newAddress = myAllocator->gcAllocate(size);
		currentObj->updateAddress((void *) newAddress);

		Object* child;
		int kids = currentObj->getPointersMax();
		currentObj->setVisited(true);
		for(i = 0 ; i < kids ; i++){
			child = currentObj->getReferenceTo(i);
			if(child && !child->getVisited()){
				child->setVisited(true);
				myQueue.push(child);
			}
		}
	}
}

CopyingCollector::~CopyingCollector() {}

} 
