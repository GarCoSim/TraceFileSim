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

#include <ctime>

extern TRACE_FILE_LINE_SIZE gLineInTrace;
extern FILE* gLogFile;
extern FILE* gDetLog;
extern FILE* traversalDepthFile;

extern FILE* gcFile;

extern clock_t start, stop;

namespace traceFileSimulator {

/** Sets initial values for internal protected values which are independent of
 * any environment options.
 *
 */
Collector::Collector() {
	shortestGC = 999999999;
	longestGC = 0;
	allGCs = 0;
}

/** Sets environment variables that can be dependant on other operations after
 * the collector has been created.
 *
 * @param allocator A pointer to the chosen collector
 * @param container A pointer to the memory manager's object container
 * @param memManager A pointer to the calling memory manager
 * @param watermark A value used to determine if a call the collector
 *        before a failed allocation is required
 * @param generation Used for creating multiple collectors
 * @param traversal Used to chose the traversal pattern
 */
void Collector::setEnvironment(Allocator* allocator, ObjectContainer* container, MemoryManager* memManager, size_t watermark, int generation, int traversal) {
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

/** The method used to collect garbage.
 * Must be implemented by subclass
 *
 * @param reason Converted to an enum and used to call the correct implementation
 */
void Collector::collect(int reason) {
}

/** Prints statistical information about the collector
 *
 */
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
	fprintf(gLogFile, "" TRACE_FILE_LINE_FORMAT " | %14s | %10zu | %14zu "
			"| %13zu | %10zu | %10zu | %10d | %4.3f | %20zu | %12zu | %21zu\n", gLineInTrace,
			statCollectionReasonString, statGcNumber, statFreedObjects,
			statLiveObjectCount, heapUsed, statFreeSpaceOnHeap, myGeneration, elapsed_secs, statFreedDuringThisGC, statCopiedObjects, statCopiedDuringThisGC);
	fflush(gLogFile);

#if DEBUG_MODE == 1 && WRITE_ALLOCATION_INFO == 1
	myAllocator->printStats();
#endif
	if(statCollectionReason == (int)reasonStatistics){
		char fl[80];
		sprintf(fl, "gen%d.log",myGeneration);
		FILE* genfile = fopen(fl,"a");

		fprintf(genfile,"%zu\n",heapUsed);
		fflush(genfile);
		fclose(genfile);
	}
	statCollectionReason = (int)reasonStatistics;
}

void Collector::addForwardingEntry(void *oldAddress, void *newAddress) {
	forwardPointers[oldAddress] = newAddress;
}

void Collector::clearForwardingEntries() {
	forwardPointers.clear();
}

/** Updates the meta data of all moved objects
 * and clears their entries
 *
 */
void Collector::updatePointers() {
	size_t i, j;
	void *pointerAddress;
	Object *currentObj;

	vector<Object *> objects = myObjectContainer->getLiveObjects();
	for (i=0; i<objects.size(); i++) {
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

/** Marks all live objects. The collector knows all live objects in advance
 *
 */
void Collector::initializeMarkPhase() {
	Object* currentObj;
	size_t i;
	vector<Object*> objects = myObjectContainer->getLiveObjects();
	for (i = 0; i < objects.size(); i++) {
		currentObj = objects[i];
		if (currentObj) {
			currentObj->setVisited(false);
		}
	}
}


/** Moves objects to the next generation.
 *
 * @return 0 on success; 1 on failure
 */
int Collector::promotionPhase() {
	if (gcsSinceLastPromotionPhase == 0) {
		//nothing to do here
		return 0;
	}
	size_t g, oldi;
	//in case the next generation is too full, a flag to wait
	int noSpaceUpstairs = 0;
	oldi = 0;
	vector<Object*> objects = myObjectContainer->getLiveObjects();
	for (g = 0; g < objects.size(); g++) {
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

/** Prints stats and increments the post collection stats
 *
 */
void Collector::postCollect() {
	printStats();
	gcsSinceLastPromotionPhase++;
}

/** Resets some collection values and increments pre collection stats
 *
 */
void Collector::preCollect(){
	start = clock();
	statFreedDuringThisGC = 0;
	statGcNumber++;
}

void Collector::lastStats() {
	//fprintf(traversalDepthFile, "\n\nOverall highes traversalDepth: %i\n", );
	//fprintf(traversalDepthFile, "Overall average traversalDepth: %i\n", );

  	std::multimap<float,int>::iterator it;
  	float totalAverageDepth = 0;
  	int totalAverageHeight = 0;

  	for (it=traversalDepthStats.begin(); it!=traversalDepthStats.end(); ++it) {
  		totalAverageDepth += (*it).first;
  		totalAverageHeight += (*it).second;
  	}
  	
  	fprintf(traversalDepthFile, "\nAverage values over all GCs:\n");
  	fprintf(traversalDepthFile, "Total average traversalDepth: %.2f\nAverage highest traversalDepth: %.2f\n\n", (float)totalAverageDepth/traversalDepthStats.size(), (float)totalAverageHeight/traversalDepthStats.size());

  	fprintf(gLogFile, "Shortest GC: %0.3fs, Longest GC: %0.3fs, Average GC time: %0.3fs\n", shortestGC, longestGC, (double)(allGCs / (statGcNumber + 1)));
}

void Collector::printTraversalDepthStats() {
	int totalTraversalDepth = 0;
	int deepestDepth = 0;

	std::map<int,int>::iterator it;
	for (it=traversalDepth.begin(); it!=traversalDepth.end(); ++it) {
		totalTraversalDepth += it->second;
		if (deepestDepth < it->second) {
			deepestDepth = it->second;
		}
	}
	float averageDepth = (float)totalTraversalDepth/traversalDepth.size();
	fprintf(traversalDepthFile, "averageDepth: %.2f\n", averageDepth);
	fprintf(traversalDepthFile, "deepestDepth: %i\n\n", deepestDepth);
	fflush(traversalDepthFile);

	traversalDepthStats.insert ( std::pair<float,int>(averageDepth,deepestDepth) );
}

/** Calls the MemoryManager::requestDelete(Object*, int) method on the object
 * pointer with a gGC of 0
 *
 * @param obj A pointer to the object we wish to free
 */
void Collector::freeObject(Object *obj) {
	if (obj) {
		myMemManager->requestDelete(obj, 0);
		statFreedObjects++;
		statFreedDuringThisGC++;
	}
}

void Collector::addCandidate(Object *obj) {

}

bool Collector::candidatesNotContainObj(Object *obj) {
	return false;
}

Collector::~Collector() {
}

}
