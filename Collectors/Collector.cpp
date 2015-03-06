/*
 * Collector.cpp
 *
 *  Created on: 2013-09-04
 *      Author: GarCoSim
 */

#include "Collector.hpp"
#include <stdio.h>
#include "../defines.hpp"

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
	statCollectionReason = 0;
	statFreedDuringThisGC = 0;

	order = (traversalEnum)traversal;
}

void Collector::collect(int reason) {
}

void Collector::checkWatermark() {

}

void Collector::printStats() {
	statFreeSpaceOnHeap = myAllocator->getFreeSize();
	int heapUsed = myAllocator->getHeapSize() - statFreeSpaceOnHeap;
	char *statCollectionReasonString;

	switch ((gcReason)statCollectionReason) {
		case reasonUnknown:
			statCollectionReasonString = (char*)"Unknown";
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
			"| %13d | %10d | %10d | %10d | %4.3f\n", gLineInTrace,
			statCollectionReasonString, statGcNumber, statFreedObjects,
			statLiveObjectCount, heapUsed, statFreeSpaceOnHeap, myGeneration, elapsed_secs);
	fflush(gLogFile);
	if (DEBUG_MODE == 1 && WRITE_ALLOCATION_INFO == 1) {
		myAllocator->printStats();
	}
	if(statCollectionReason == 0){
		char fl[80];
		sprintf(fl, "gen%d.log",myGeneration);
		FILE* genfile = fopen(fl,"a");

		fprintf(genfile,"%d\n",heapUsed);
		fflush(genfile);
		fclose(genfile);
	}
	statCollectionReason = 0;
}

void Collector::postCollect() {
	printStats();
	gcsSinceLastPromotionPhase++;
}


int Collector::promotionPhase() {
	return -1;
}

void Collector::lastStats() {
	fprintf(gLogFile, "Shortest GC: %0.3fs, Longest GC: %0.3fs, Average GC time: %0.3fs\n", shortestGC, longestGC, (double)(allGCs / statGcNumber));
}

Collector::~Collector() {
}

} 
