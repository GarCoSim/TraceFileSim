/*
 * RegionBasedAllocator.cpp
 *
 *  Created on: 2015-11-03
 *      Author: GarCoSim
 *
 */

#include "RegionBasedAllocator.hpp"

extern int gLineInTrace;
extern FILE* balancedLogFile;
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
		currentFreeSpace = balancedRegions[i]->getCurrFree();
		
		if (size <= currentFreeSpace) {
			edenRegionID = edenRegions.at(i);
			currentFreeAddress = balancedRegions[edenRegionID]->getCurrFreeAddr();	
			balancedRegions[edenRegionID]->setCurrFreeAddr((void*)((long)currentFreeAddress+(long)size));
			balancedRegions[edenRegionID]->setCurrFree(currentFreeSpace-size);
			balancedRegions[edenRegionID]->incrementObjectCount();
			
			setAllocated((long)currentFreeAddress, size);
			fprintf(balancedLogFile, "Allocated %zu bytes to Eden Region %i at address %ld\n", size, edenRegionID, (long)currentFreeAddress);
			return &heap[(long)currentFreeAddress];
		}
	}
	
	if (currentNumberEdenRegions < maxNumberOfEdenRegions) {

		if (freeRegions.size() > 0) {
			edenRegionID = freeRegions.front();
			currentFreeSpace = balancedRegions[edenRegionID]->getCurrFree();
			
			if (size <= currentFreeSpace) {
				currentFreeAddress = balancedRegions[edenRegionID]->getCurrFreeAddr();
				balancedRegions[edenRegionID]->setCurrFreeAddr((void*)((long)currentFreeAddress+(long)size));
				balancedRegions[edenRegionID]->setCurrFree(currentFreeSpace-size);
				balancedRegions[edenRegionID]->incrementObjectCount();
				
				freeRegions.erase(freeRegions.begin());
				edenRegions.push_back(edenRegionID);
				
				setAllocated((long)currentFreeAddress, size);
				fprintf(balancedLogFile, "Allocated %zu bytes to Eden Region %i at address %ld\n", size, edenRegionID, (long)currentFreeAddress);
				return &heap[(long)currentFreeAddress];
			}
			else if (size > currentFreeSpace) {
				fprintf(stderr, "Allocation for arraylets not yet implemented!\n");
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
