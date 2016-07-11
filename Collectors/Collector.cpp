/*
 * Collector.cpp
 *
 *  Created on: 2013-09-04
 *      Author: GarCoSim
 */

#include "Collector.hpp"
#include "../Allocators/Allocator.hpp"
#include "../Main/ObjectContainer.hpp"
#include "../WriteBarriers/WriteBarrier.hpp"
#include "../Main/MemoryManager.hpp"

extern int gLineInTrace;
extern FILE* gLogFile;
extern FILE* gDetLog;

extern FILE* gcFile;

extern clock_t start, stop;

namespace traceFileSimulator {

Collector::Collector() {
	shortestGC = 999999999;
	longestGC = 0;
	allGCs = 0;
}

void Collector::setEnvironment(Allocator* allocator, ObjectContainer* container, MemoryManager* memManager, int watermark, int generation, int traversal) {
	myAllocator = allocator;
	myObjectContainer = container;
	myWatermark = watermark;
	myMemManager = memManager;
	myGeneration = generation;
	gcsSinceLastPromotionPhase = 0;
	myTraversal = traversal;
	//stats
	statFreedObjects = 0;
	statGcNumber = 0;
	statFreeSpaceFragmentCount = 0;
	statFreeSpaceOnHeap = 0;
	statLiveObjectCount = 0;
	statCollectionReason = (int)reasonStatistics;
	statFreedDuringThisGC = 0;

	statCopiedDuringThisGC = 0;
	statCopiedObjects = 0;

	order = (traversalEnum)traversal;
}

void Collector::checkWatermark() {
	size_t size = myAllocator->getRegionSize();
	size_t free = myAllocator->getFreeSize();
	size_t ratio = 100 - (100 * free / size);
	if (ratio > myWatermark) {
		collect((int)reasonHighWatermark);
	}
}

void Collector::printStats() {
	statFreeSpaceOnHeap = myAllocator->getFreeSize();
	size_t heapUsed = myAllocator->getHeapSize() - statFreeSpaceOnHeap;
	char *statCollectionReasonString;

	switch ((gcReason)statCollectionReason) {
		case reasonStatistics:
			statCollectionReasonString = (char*)"Statistics";
			break;
		case reasonFailedAlloc:
			statCollectionReasonString = (char*)"Failed Alloc";
			break;
		case reasonHighWatermark:
			statCollectionReasonString = (char*)"High Watermark";
			break;
		case reasonDebug:
			statCollectionReasonString = (char*)"Debug";
			break;
		case reasonShift:
			statCollectionReasonString = (char*)"Shift";
			break;
		case reasonEval:
			statCollectionReasonString = (char*)"Evaluation";
			break;
		case reasonForced:
			statCollectionReasonString = (char*)"Forced";
			break;
		default:
			statCollectionReasonString = (char*)"ERROR";
			break;
	}

	stop = clock();
	double elapsed_secs = double(stop - start)/CLOCKS_PER_SEC;
	if (elapsed_secs < shortestGC)
		shortestGC = elapsed_secs;
	if (elapsed_secs > longestGC)
		longestGC = elapsed_secs;
	allGCs += longestGC;

	statLiveObjectCount = myObjectContainer->countElements();
	fprintf(gLogFile, "%8d | %14s | %10d | %14d "
			"| %13d | %10zu | %10d | %10d | %4.3f | %20d | %12d | %21d\n", gLineInTrace,
			statCollectionReasonString, statGcNumber, statFreedObjects, 
			statLiveObjectCount, heapUsed, statFreeSpaceOnHeap, myGeneration, elapsed_secs, statFreedDuringThisGC, statCopiedObjects, statCopiedDuringThisGC);
	fflush(gLogFile);
	if (DEBUG_MODE == 1 && WRITE_ALLOCATION_INFO == 1) {
		myAllocator->printStats();
	}
	statCollectionReason = (int)reasonStatistics;
}

void Collector::addForwardingEntry(void *oldAddress, void *newAddress) {
	forwardPointers[oldAddress] = newAddress;
}

void Collector::clearForwardingEntries() {
	forwardPointers.clear();
}

void Collector::updatePointers() {
	int i, j;
	void *pointerAddress;
	Object *currentObj;

	vector<Object *> objects = myObjectContainer->getLiveObjects();
	for (i=0; i<(int)objects.size(); i++) {
		currentObj = objects[i];
		for (j=0; j<currentObj->getPointersMax(); j++) {
			pointerAddress = currentObj->getRawPointerAddress(j);
			if (pointerAddress != NULL && forwardPointers.find(pointerAddress) != forwardPointers.end())
				currentObj->setRawPointerAddress(j, forwardPointers[pointerAddress]);
		}
		currentObj->setForwarded(false);
	}

	clearForwardingEntries();
}

void Collector::initializeMarkPhase() {
	Object* currentObj;
	int i;
	vector<Object*> objects = myObjectContainer->getLiveObjects();
	for (i = 0; i < (int)objects.size(); i++) {
		currentObj = objects[i];
		if (currentObj) {
			currentObj->setVisited(false);
		}
	}
}

void Collector::compact() {
	/*only alive objects are left in the container. If I traverse
	 through the live list, I get all elements*/
	//free everything.
	myMemManager->statBeforeCompact(myGeneration);
	freeAllLiveObjects();
	//allocate everything back.
	myMemManager->requestResetAllocationPointer(myGeneration);
	reallocateAllLiveObjects();
	myMemManager->statAfterCompact(myGeneration);
}

int Collector::promotionPhase() {
	if (gcsSinceLastPromotionPhase == 0) {
		//nothing to do here
		return 0;
	}
	int g, oldi;
	//in case the next generation is too full, a flag to wait
	int noSpaceUpstairs = 0;
	oldi = -1;
	vector<Object*> objects = myObjectContainer->getLiveObjects();
	for (g = 0; g < (int)objects.size(); g++) {
		Object* currentObj = objects[g];
		if (g < oldi) {
			printf("oO\n");
		}
		//if object exists and is not in the oldest generation already
		if (currentObj && currentObj->getGeneration() < (GENERATIONS - 1)) {
			int age = currentObj->getAge();
			if (age
					>= PROMOTIONAGE + PROMOTIONAGE * myGeneration
							/*+ currentObj->getGeneration() * PROMOTIONAGEFACTOR*/) {
				//fprintf(stderr, "(%d) promoting object of age %d\n",gLineInTrace, age);
				if (currentObj->getGeneration() < GENERATIONS - 1) {
					noSpaceUpstairs = myMemManager->requestPromotion(
							currentObj);
					// end the promotion phase because of an "out of memory"
					// exception in the higher generation. It is not the best
					// solution
					if (noSpaceUpstairs == 1) {
						return -1;
					}
				}
			}
		}
		oldi = g;
	}
	return 0;
}

void Collector::initializeHeap() {
}

void Collector::postCollect() {
	printStats();
	gcsSinceLastPromotionPhase++;
}

void Collector::preCollect(){
	start = clock();
	statFreedDuringThisGC = 0;
	statGcNumber++;
}

void Collector::lastStats() {
	fprintf(gLogFile, "Shortest GC: %0.3fs, Longest GC: %0.3fs, Average GC time: %0.3fs\n", shortestGC, longestGC, (double)(allGCs / (statGcNumber + 1)));   
}

void Collector::freeObject(Object *obj) {
	if (obj) {
		//fprintf(stderr, "Freeing %i in freeObject\n", obj->getID());
		myMemManager->requestDelete(obj, 0);
		statFreedObjects++;
		statFreedDuringThisGC++;
	}
}
void Collector::freeAllLiveObjects(){
	int i;
	vector<Object*> objects = myObjectContainer->getLiveObjects();
	for (i = 0; i < (int)objects.size(); i++) {
		Object* currentObj = objects[i];
		if (currentObj) {
			myMemManager->requestFree(currentObj);
		}
	}
}

void Collector::reallocateAllLiveObjects() {
	int i;
	void *addressBefore, *addressAfter;
	// the ordering ensures that we don't overwrite an object we have yet to visit
	vector<Object*> objects = myObjectContainer->getLiveObjectsInHeapOrder();
	for (i = 0; i < (int)objects.size(); i++) {
		Object* currentObj = objects[i];
		if (currentObj) {
			addressBefore = currentObj->getAddress();
			myMemManager->requestReallocate(currentObj);
			addressAfter = currentObj->getAddress();
			addForwardingEntry(addressBefore, addressAfter);
		}
	}
}

void Collector::addCandidate(Object *obj) {

}

bool Collector::candidatesNotContainObj(Object *obj) {
	return false;
}

bool Collector::candidatesContainObj(Object *obj) {
	return false;
}

void Collector::removeObjectFromCandidates(Object *obj) {
	
}

Collector::~Collector() {
}

} 
