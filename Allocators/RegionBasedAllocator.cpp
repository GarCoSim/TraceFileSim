/*
 * RegionBasedAllocator.cpp
 *
 *  Created on: 2015-11-03
 *      Author: GarCoSim
 *
 */

#include "RegionBasedAllocator.hpp"

extern int gLineInTrace;
using namespace std;


namespace traceFileSimulator {

RegionBasedAllocator::RegionBasedAllocator() {
}

bool RegionBasedAllocator::isRealAllocator() {
	return true;
}


void RegionBasedAllocator::initializeHeap(size_t heapSize) {
	overallHeapSize = heapSize;
	myHeapBitMap = new char[(size_t)ceil(heapSize/8.0)];
	heap = (unsigned char*)malloc(heapSize); // * 8
	statLiveObjects = 0;
	resetRememberedAllocationSearchPoint();

	if (DEBUG_MODE && WRITE_ALLOCATION_INFO) {
		allocLog = fopen("alloc.log", "w+");
	}
	if (DEBUG_MODE && WRITE_HEAPMAP) {
		heapMap = fopen("heapmap.log", "w+");
	}
	fprintf(stderr, "heap size %zd\n", overallHeapSize);
}

void RegionBasedAllocator::freeAllSectors() {
	unsigned int i;
	for (i = 0; i < overallHeapSize; i++) {
		setBitUnused(i);
	}

}

void *RegionBasedAllocator::allocate(size_t size, size_t lower, size_t upper) {

	if (size <= 0)
		return NULL;
	
	unsigned int i, currentNumberEdenRegions, edenRegionID;
	size_t currentFreeSpace;
	void *currentFreeAddress;
	
	currentNumberEdenRegions = edenRegions.size();

	for (i = 0; i < currentNumberEdenRegions; i++) {
		edenRegionID = edenRegions.at(i);
		currentFreeSpace = balancedGCRegions[i]->getCurrFree();
		
		if (size <= currentFreeSpace) {
			currentFreeAddress = balancedGCRegions[edenRegionID]->getCurrFreeAddr();	
			balancedGCRegions[edenRegionID]->setCurrFreeAddr((void*)((long)currentFreeAddress+(long)size));
			balancedGCRegions[edenRegionID]->setCurrFree(currentFreeSpace-size);
			balancedGCRegions[edenRegionID]->incNumObj();
			
			setAllocated((long)currentFreeAddress, size);
			return &heap[(long)currentFreeAddress];
		}
	}
	
	if (currentNumberEdenRegions < maxNumberOfEdenRegions) {

		if (freeRegions.size() > 0) {
			edenRegionID = freeRegions.front();
			currentFreeSpace = balancedGCRegions[edenRegionID]->getCurrFree();
			
			if (size <= currentFreeSpace) {
				currentFreeAddress = balancedGCRegions[edenRegionID]->getCurrFreeAddr();
				balancedGCRegions[edenRegionID]->setCurrFreeAddr((void*)((long)currentFreeAddress+(long)size));
				balancedGCRegions[edenRegionID]->setCurrFree(currentFreeSpace-size);
				balancedGCRegions[edenRegionID]->incNumObj();
				
				freeRegions.erase(freeRegions.begin());
				edenRegions.push_back(edenRegionID);
				
				setAllocated((long)currentFreeAddress, size);
				return &heap[(long)currentFreeAddress];
			}
		}
	}

	return NULL;
}

size_t RegionBasedAllocator::getHeapIndex(Object *object) {
	// This conversion is only valid because the heap is an array of bytes.
	return (size_t) ((char *) object->getAddress() - (char *) heap);
}


void RegionBasedAllocator::gcFree(Object* object) {
	size_t size = object->getHeapSize();
	size_t heapIndex = getHeapIndex(object);

	setFree(heapIndex, size);
	statLiveObjects--;
}

RegionBasedAllocator::~RegionBasedAllocator() {
}

void RegionBasedAllocator::freeOldSpace() {
	setFree(oldSpaceStartHeapIndex, oldSpaceEndHeapIndex-oldSpaceStartHeapIndex);
}

}
