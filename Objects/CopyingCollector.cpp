/*
 * CopyingCollector.cpp
 *
 *  Created on: 2013-09-04
 *      Author: GarCoSim
 */

#include "CopyingCollector.hpp"
#include <stdio.h>
#include "../defines.hpp"

extern int gLineInTrace;
extern FILE* gLogFile;

namespace traceFileSimulator {

CopyingCollector::CopyingCollector(Allocator* allocator, ObjectContainer* container){
	myAllocator = allocator;
	myObjectContainer = container;
	//stats
	statFreedObjects = 0;
	statGcNumber = 0;
	statFreeSpaceFragmentCount = 0;
	statFreeSpaceOnHeap = 0;
	statLiveObjectCount = 0;
	statCollectionReason = 0;
	statFreedDuringThisGC = 0;
	statHeapSide = 0;
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
//		collect(2);
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
		currentObj->setIsAlive(1);
		for(i = 0 ; i < kids ; i++){
			child = currentObj->getReferenceTo(i);
			if(child && child->getVisited() == 0){
				child->setVisited(1);
				currentObj->setIsAlive(1);
				myQueue.push(child);
			}
		}
	}

}

void CopyingCollector::sweep(){
	Object* currentObj;
	int i;
	for(i = 0 ; i < myObjectContainer->getSize() ; i++){
		currentObj = myObjectContainer->getbySlotNr(i);
		if(currentObj && currentObj->getIsAlive() == 0){
			myAllocator->gcFree(currentObj);
			myObjectContainer->deleteObject(currentObj);
			statFreedObjects++;
			statFreedDuringThisGC++;
		}
	}
}

void CopyingCollector::enqueueAllRoots(){
	Object* currentObj;
	int i, j;

	for(i = 0 ; i < NUM_THREADS ; i++){
		for(j = 0 ; j < ROOTSET_SIZE ; j++){
			currentObj = myObjectContainer->getRoot(i,j);
			if(currentObj && currentObj->getVisited() == 0){
				currentObj->setVisited(1);
				currentObj->setIsAlive(1);
				myQueue.push(currentObj);
			}
		}
	}
}

void CopyingCollector::initializeMarkPhase(){
	Object* currentObj;
	int i;
	for(i = 0 ; i < myObjectContainer->getSize() ; i++){
		currentObj = myObjectContainer->getbySlotNr(i);
		if(currentObj != NULL){
			currentObj->setVisited(0);
			currentObj->setIsAlive(0);
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
	myAllocator->setAllocationSeearchStart(0);
	reallocateAllLiveObjects();
}

void CopyingCollector::postCollect(){
	printStats();
}

void CopyingCollector::printStats(){
	statFreeSpaceOnHeap = myAllocator->getFreeSize();
	int heapUsed = myAllocator->getHeapSize() - statFreeSpaceOnHeap;
	statLiveObjectCount = myObjectContainer->countElements();
	fprintf(gLogFile, "%8d | %9d | %10d | %14d "
			"| %13d | %10d | %10d |\n",
			 gLineInTrace, statCollectionReason, statGcNumber,
			 statFreedObjects, statLiveObjectCount, heapUsed, statFreeSpaceOnHeap);
	if(DEBUG_MODE == 1 && WRITE_ALLOCATION_INFO == 1){
		myAllocator->printStats();
	}
	statCollectionReason = 0;
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
		currentObj->setIsAlive(1);
		for(i = 0 ; i < kids ; i++){
			child = currentObj->getReferenceTo(i);
			if(child && child->getVisited() == 0){
				child->setVisited(1);
				currentObj->setIsAlive(1);
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
		currentObj->updateAddress(newAddress);

		Object* child;
		int kids = currentObj->getPointersMax();
		currentObj->setIsAlive(1);
		for(i = 0 ; i < kids ; i++){
			child = currentObj->getReferenceTo(i);
			if(child && child->getVisited() == 0){
				child->setVisited(1);
				currentObj->setIsAlive(1);
				myQueue.push(child);
			}
		}
	}
}

CopyingCollector::~CopyingCollector() {}

} 
