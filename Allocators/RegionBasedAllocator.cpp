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

	if(numberOfRegions >= MAXREGIONS || overallHeapSize == maximumHeapSize){
		return -1; //can't add more regions, at maximum
	}

	// Calculate new number of regions to create
	newNumberOfRegions = numberOfRegions;
	if((newNumberOfRegions + numberOfRegions) > MAXREGIONS){ //check if surpassing maximum number of regions
		newNumberOfRegions = MAXREGIONS - numberOfRegions;
	}
	newHeapSize = newNumberOfRegions * regionSize;
	if((newHeapSize + overallHeapSize) > maximumHeapSize){ //check if surpassing maximum heap size
		newNumberOfRegions = (maximumHeapSize - overallHeapSize) / regionSize;
		newHeapSize = newNumberOfRegions * regionSize;
	}

	if(newHeapSize == 0){
		return -1; // Can't add more regions, not enough space for a region
	}

	fprintf(stdout, "Adding more regions\n");

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
	fprintf(balancedLogFile, "\nStatistics After Adding Regions:\n\n");
	fprintf(balancedLogFile, "Heap Size = %zu\n", overallHeapSize);
	fprintf(balancedLogFile, "Region Size = %zu\n", regionSize);
	fprintf(balancedLogFile, "Number of Regions = %i\n", numberOfRegions);
	fprintf(balancedLogFile, "Maximum number of Eden Regions = %i\n\n", maxNumberOfEdenRegions);

	return 0;
}

// Merge adjacent regions
int RegionBasedAllocator::mergeRegions(){
	std::vector<heapStats>::iterator it;
	heapStats currentHeap;
	unsigned int i;
	std::set<void*> currentRemset;
	void* currentAddress;

	if(!maximumMerges){
		fprintf(stderr, "Already merged maximum times\n");
		return -1;
	}
	// Check if any heaps have an odd number of regions
	// TODO replace with check for surpassing max allowable merges (calculated during Allocator::setNumberOfRegionsHeap)
	for(it = allHeaps.begin(); it != allHeaps.end(); ++it){
		currentHeap = *it;
		if(((currentHeap.heapEnd - currentHeap.heapStart)/regionSize)%2){
			fprintf(stderr, "Odd number of regions\n");
			return -1;
		}
	}

	printf("Merging regions\n");

	// Update ID of first region in each heap
	for(it = allHeaps.begin(); it != allHeaps.end(); ++it){
		(*it).firstRegion = (*it).firstRegion/2;
	}

	// Update statistics
	numberOfRegions = numberOfRegions/2;
	maxNumberOfEdenRegions = (int)(ceil((EDENREGIONS * (double)numberOfRegions)/100));
	regionSize = regionSize * 2;
	printf("New region size: %zu\n", regionSize);

	// Empty free regions and eden regions vectors
	freeRegions.clear();
	edenRegions.clear();

	// Merge regions and update statistics
	for(i = 0; i < balancedRegions.size(); i++){
		balancedRegions[i]->setSize(regionSize);
		balancedRegions[i]->setNumObj(balancedRegions[i]->getNumObj() + balancedRegions[i+1]->getNumObj());
		balancedRegions[i]->setCurrFree(balancedRegions[i]->getCurrFree() + balancedRegions[i+1]->getCurrFree());
		if(balancedRegions[i+1]->getNumObj() != 0){
			balancedRegions[i]->setCurrFreeAddr(balancedRegions[i+1]->getCurrFreeAddr());
			balancedRegions[i]->setCurrFree(balancedRegions[i+1]->getCurrFree());
			balancedRegions[i]->setAge((balancedRegions[i]->getAge() < balancedRegions[i+1]->getAge())?balancedRegions[i]->getAge():balancedRegions[i+1]->getAge());//setAge to min of either region
		}

		// Combine remsets, remove and references between merged regions
		std::set<void*>::iterator it;
		currentRemset = balancedRegions[i]->getRemset();
		for(it = currentRemset.begin(); it != currentRemset.end(); ++it){
			currentAddress = *it;
			if(currentAddress > balancedRegions[i]->getAddress() && currentAddress < (void*)((char*)balancedRegions[i]->getAddress() + regionSize)){
				balancedRegions[i]->eraseObjectReferenceWithoutCheck(currentAddress);
			}
		}
		currentRemset = balancedRegions[i+1]->getRemset();
		for(it = currentRemset.begin(); it != currentRemset.end(); ++it){
			currentAddress = *it;
			if(currentAddress < balancedRegions[i]->getAddress() || currentAddress > (void*)((char*)balancedRegions[i]->getAddress() + regionSize)){
				balancedRegions[i]->insertObjectReference(currentAddress);
			}
		}

		// Add to edenRegions or freeRegions, as necessary
		if(balancedRegions[i]->getAge() == 0 && balancedRegions[i]->getNumObj() != 0){ //if the merged region has objects of age 0, add to list of eden regions
			edenRegions.push_back(i);
		}
		if(balancedRegions[i]->getNumObj() == 0){ //if the merged region has no objects, add to list of free regions
			freeRegions.push_back(i);
		}

		balancedRegions.erase(balancedRegions.begin()+i+1); //remove second region from list of all regions
	}

	fprintf(balancedLogFile, "\nStatistics After Merging:\n\n");
	fprintf(balancedLogFile, "Heap Size = %zu\n", overallHeapSize);
	fprintf(balancedLogFile, "Region Size = %zu\n", regionSize);
	fprintf(balancedLogFile, "Number of Regions = %i\n", numberOfRegions);
	fprintf(balancedLogFile, "Maximum number of Eden Regions = %i\n\n", maxNumberOfEdenRegions);

	maximumMerges--;
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


}
