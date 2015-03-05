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
		default:
			statCollectionReasonString = (char*)"ERROR";
			break;
	}

	statLiveObjectCount = myObjectContainer->countElements();
	fprintf(gLogFile, "%8d | %14s | %10d | %14d "
			"| %13d | %10d | %10d | %5d |\n", gLineInTrace,
			statCollectionReasonString, statGcNumber, statFreedObjects,
			statLiveObjectCount, heapUsed, statFreeSpaceOnHeap, myGeneration);
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

int Collector::promotionPhase() {
	return -1;
}

Collector::~Collector() {
}

} 
