/*
 * Collector.cpp
 *
 *  Created on: 2013-09-04
 *      Author: GarCoSim
 */

#include "Collector.hpp"
#include <stdio.h>
#include "../defines.hpp"


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

}

int Collector::promotionPhase() {
	return -1;
}

Collector::~Collector() {
}

} 
