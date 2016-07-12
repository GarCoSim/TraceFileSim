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

void RegionBasedAllocator::initializeHeap(size_t heapSize, size_t maxHeapSize) {
	overallHeapSize = heapSize;
	maximumHeapSize = maxHeapSize;
	statLiveObjects = 0;
	resetRememberedAllocationSearchPoint();

	if (DEBUG_MODE && WRITE_ALLOCATION_INFO) {
		allocLog = fopen("alloc.log", "w+");
	}
	if (DEBUG_MODE && WRITE_HEAPMAP) {
		heapMap = fopen("heapmap.log", "w+");
	}
	fprintf(stderr, "heap size %zd\n", overallHeapSize);
	allHeaps.push_back(heap);
	allHeaps.push_back(heap + overallHeapSize);
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
	unsigned char *currentHeap;

	currentNumberEdenRegions = edenRegions.size();

	for (i = 0; i < currentNumberEdenRegions; i++) {
		edenRegionID = edenRegions[i];
		currentFreeSpace = balancedRegions[edenRegionID]->getCurrFree();

		if (size <= currentFreeSpace) {
			currentHeap = balancedRegions[edenRegionID]->getHeapAddress();
			currentFreeAddress = balancedRegions[edenRegionID]->getCurrFreeAddr();
			balancedRegions[edenRegionID]->setCurrFreeAddr((void*)((long)currentFreeAddress+(long)size));
			//fprintf(balancedLogFile, "Bla: %zu\n", currentFreeSpace-size);
			balancedRegions[edenRegionID]->setCurrFree(currentFreeSpace-size);
			balancedRegions[edenRegionID]->incrementObjectCount();

			setAllocated(currentHeap, (long)currentFreeAddress, size);

			//fprintf(balancedLogFile, "Allocated %zu bytes to Eden Region %i at address %ld. currentFreeSpace = %zu \n", size, edenRegionID, (long)currentFreeAddress, currentFreeSpace);
			//fprintf(balancedLogFile, "Allocated to: %i. ", edenRegionID);


			return &currentHeap[(long)currentFreeAddress];
		}
	}

	if (currentNumberEdenRegions < maxNumberOfEdenRegions) {

		if (freeRegions.size() > 0) {
			edenRegionID = freeRegions.front();
			currentFreeSpace = balancedRegions[edenRegionID]->getCurrFree();

			if (size <= currentFreeSpace) {
				currentHeap = balancedRegions[edenRegionID]->getHeapAddress();
				currentFreeAddress = balancedRegions[edenRegionID]->getCurrFreeAddr();
				balancedRegions[edenRegionID]->setCurrFreeAddr((void*)((long)currentFreeAddress+(long)size));
				balancedRegions[edenRegionID]->setCurrFree(currentFreeSpace-size);
				balancedRegions[edenRegionID]->incrementObjectCount();

				freeRegions.erase(freeRegions.begin());
				edenRegions.push_back(edenRegionID);

				setAllocated(currentHeap, (long)currentFreeAddress, size);

				//fprintf(balancedLogFile, "Allocated %zu bytes to Eden Region %i at address %ld\n", size, edenRegionID, (long)currentFreeAddress);
				//fprintf(balancedLogFile, "Allocated to: %i\n", edenRegionID);


				return &currentHeap[(long)currentFreeAddress];
			}
			else if (size > currentFreeSpace) {
				fprintf(stderr, "Allocation for arraylets not yet implemented!\n");
			}
		}
	}

	return NULL;
}


// Attempt to add more regions when no more free regions exist
int RegionBasedAllocator::addRegions(){
	size_t newHeapSize;
	unsigned int i, newNumberOfRegions;
	unsigned char *newHeap;
	Region* balancedRegion;
	size_t currentAddress = 0;
	char* newHeapBitMap;

	if(numberOfRegions >= MAXREGIONS){
		return -1; //can't add more regions, at maximum
	}

	fprintf(stdout, "Adding more regions\n");

	// Calculate new number of regions to create
	newNumberOfRegions = numberOfRegions;
	if((newNumberOfRegions + numberOfRegions) > MAXREGIONS){ //check if surpassing maximum number of regions
		newNumberOfRegions = MAXREGIONS - numberOfRegions;
	}
	newHeapSize = newNumberOfRegions * regionSize;
	if((newHeapSize + overallHeapSize) > maximumHeapSize){ //check if surpassing maximum heap size
		newHeapSize = maximumHeapSize - overallHeapSize;
		newNumberOfRegions = newHeapSize / regionSize;
	}

	newHeap = (unsigned char*)malloc(newHeapSize); //create new heap
	if(newHeap == NULL){
		fprintf(stderr, "Out of memory\n");
		return -1;
	}

	allHeaps.push_back(newHeap);
	allHeaps.push_back(newHeap + newHeapSize);

	// Initialize new regions
	newNumberOfRegions += numberOfRegions;
	for (i = numberOfRegions; i < newNumberOfRegions; i++) {
		balancedRegion = new Region ((void*)currentAddress, regionSize, newHeap);

		balancedRegions.push_back(balancedRegion);
		freeRegions.push_back(i);

		currentAddress = currentAddress + regionSize;
	}

	// Increase bitmap
	newHeapBitMap = new char[(size_t)ceil((overallHeapSize + newHeapSize)/8.0)];
	memcpy(newHeapBitMap, myHeapBitMap, ceil(overallHeapSize/8.0));
	delete(myHeapBitMap);
	myHeapBitMap = newHeapBitMap;

	// Update statistics
	overallHeapSize += newHeapSize;
	numberOfRegions = newNumberOfRegions;
	maxNumberOfEdenRegions = (int)(ceil((EDENREGIONS * (double)numberOfRegions)/100));

	fprintf(stdout, "New heap size: %zd\n", overallHeapSize);
	fprintf(balancedLogFile, "\nNew Region Statistics:\n\n");
	fprintf(balancedLogFile, "Heap Size = %zu\n", overallHeapSize);
	fprintf(balancedLogFile, "Region Size = %zu\n", regionSize);
	fprintf(balancedLogFile, "Number of Regions = %i\n", numberOfRegions);
	fprintf(balancedLogFile, "Maximum number of Eden Regions = %i\n\n", maxNumberOfEdenRegions);

	return 0;
}

// Updated for multiple heaps
size_t RegionBasedAllocator::getHeapIndex(Object *object) {
	// This conversion is only valid because the heap is an array of bytes.
	void *objectAddress = object->getAddress();
	unsigned int i, j;
	char *offset;

	for(i=0; i<allHeaps.size(); i+=2){
		if(objectAddress >= &allHeaps.at(i)[0] && objectAddress < &allHeaps.at(i+1)[0]){
			for(j=0; j<i; j+=2){
				offset += ((char *)&allHeaps.at(j+1)[0] - (char *)&allHeaps.at(j)[0]);
			}
			return (size_t)((char *)objectAddress - (char *)allHeaps.at(i) + offset);
		}
	}
	printf("getHeapIndex\n");
	return -1;
	//return (size_t) ((char *) object->getAddress() - (char *) allHeaps.at(0));
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
