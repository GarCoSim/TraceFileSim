/*
 * Allocator.cpp
 *
 *  Created on: 2013-09-03
 *      Author: GarCoSim
 */

#include "Allocator.hpp"
#include "../Main/Object.hpp"
#include <stdio.h>
#include <stdlib.h>
#include "../defines.hpp"
#include <math.h>

extern int gLineInTrace;
extern FILE* balancedLogFile;

using namespace std;

namespace traceFileSimulator {

//keep setHalfHeapSize() for now, replace it eventually with setNumberOfRegionsHeap
void Allocator::setHalfHeapSize(bool value) {
	if (value) {
		oldSpaceStartHeapIndex = 0;
		oldSpaceEndHeapIndex = overallHeapSize / 2;
		newSpaceStartHeapIndex = oldSpaceEndHeapIndex;
		newSpaceEndHeapIndex = overallHeapSize;
		regionSize = overallHeapSize/2;
		isSplitHeap = true;
	}
	else {
		oldSpaceStartHeapIndex = 0;
		oldSpaceEndHeapIndex = overallHeapSize;
		regionSize = overallHeapSize;
		isSplitHeap = false;
	}
}

void Allocator::setNumberOfRegionsHeap(int value) {

	if (value == 2) {
		numberOfRegions = value;
		regionSize = overallHeapSize/value;
		oldSpaceStartHeapIndex = 0;
		oldSpaceEndHeapIndex = overallHeapSize / 2;
		newSpaceStartHeapIndex = oldSpaceEndHeapIndex;
		newSpaceEndHeapIndex = overallHeapSize;
		isSplitHeap = true;
	}
	else if (value == 1) {
		numberOfRegions = value;
		regionSize = overallHeapSize;
		oldSpaceStartHeapIndex = 0;
		oldSpaceEndHeapIndex = overallHeapSize;
		isSplitHeap = false;
	}
	else if (value == 0) {
		// Determine numberOfRegions and regionSize based on overallHeapSize
		unsigned int i, currentNumberOfRegions, currentMaximumRegions;
		size_t currentRegionSize, maximumRegionSize;
		currentRegionSize = 0;
		currentNumberOfRegions = 0;
		bool initialStop = false;
		bool maxStop = false;
		heapStats currentHeap;

		for (i = REGIONEXPONENT; !(initialStop && maxStop); i++) {
			currentRegionSize = (size_t)pow((float)2,(float)i) * MAGNITUDE_CONVERSION; //KB to Byte
			currentNumberOfRegions = overallHeapSize/currentRegionSize;
			currentMaximumRegions = maximumHeapSize/currentRegionSize;

			if (currentNumberOfRegions <= MAXREGIONS && !initialStop){
				regionSize = currentRegionSize;
				initialStop = true;
			}
			if (currentMaximumRegions <= MAXREGIONS && !maxStop){
				maximumRegionSize = currentRegionSize;
				maxStop = true;
			}
			if (i >= 24){ //2^24 = 16.8GB, should be more than enough space per region
				initialStop = true;
				maxStop = true;
			}
		}

		// Calculate initial number of regions
		numberOfRegions = overallHeapSize/regionSize;
		if(numberOfRegions < 1){
			numberOfRegions = 1;
		}

		// Calculate maximum region merges
		maximumMerges = log2(maximumRegionSize/regionSize);
		numberOfRegions += numberOfRegions%(int)pow(2, maximumMerges);

		overallHeapSize = numberOfRegions * regionSize;
		maxNumberOfEdenRegions = (int)(ceil((EDENREGIONS * (double)numberOfRegions)/100));

		// Create heap
		heap = (unsigned char*)malloc(overallHeapSize);
		currentHeap.heapStart = heap;
		currentHeap.heapEnd = heap+overallHeapSize;
		currentHeap.firstRegion = 0;
		allHeaps.push_back(currentHeap);

		// Create the bitmap
		myHeapBitMap = new char[(size_t)ceil(overallHeapSize/8.0)];

		//initialize regions
		Region* balancedRegion;
		size_t currentAddress = 0;

		for (i = 0; i < numberOfRegions; i++) {
			balancedRegion = new Region ((void*)currentAddress, regionSize, heap);

			balancedRegions.push_back(balancedRegion);
			freeRegions.push_back(i);

			currentAddress = currentAddress + regionSize;
		}

		fprintf(balancedLogFile, "Region Statistics:\n\n");
		fprintf(balancedLogFile, "Heap Size = %zu\n", overallHeapSize);
		fprintf(balancedLogFile, "Region Size = %zu\n", regionSize);
		fprintf(balancedLogFile, "Number of Regions = %i\n", numberOfRegions);
		fprintf(balancedLogFile, "Maximum number of Eden Regions = %i\n", maxNumberOfEdenRegions);
		fprintf(balancedLogFile, "Maximum number of Merges = %i \n\n\n", maximumMerges);
		fflush(balancedLogFile);
	}
}

Allocator::Allocator() {
}

size_t Allocator::getFreeSize() {
	size_t i;
	size_t count = 0;
	for (i=0; i<overallHeapSize; i++)
		if (!isBitSet(i))
			count++;

	return count;
}

unsigned char *Allocator::getHeap() {
	return heap;
}

size_t Allocator::getOldSpaceStartHeapIndex(){
	return oldSpaceStartHeapIndex;
}

size_t Allocator::getOldSpaceEndHeapIndex(){
	return oldSpaceEndHeapIndex;
}

size_t Allocator::getSpaceToNextObject(size_t start){
	for(size_t i=start; i<getRegionSize(); i++){
		if(isBitSet(i)){
			return i-start;
		}
	}
	ERRMSG("Object found in getNextObjectAddress(), but here it doesn't exist.\n");
	exit (1);
}

unsigned char *Allocator::getNextObjectAddress(size_t start){
	for(size_t i=start; i<getRegionSize(); i++){
		if(isBitSet(i)){
			return &heap[i];
		}
	}
	return NULL;
}

void *Allocator::gcAllocate(size_t size) {
	return allocate(size, oldSpaceStartHeapIndex, oldSpaceEndHeapIndex);
}

void *Allocator::allocateInNewSpace(size_t size) {
	return allocate(size, newSpaceStartHeapIndex, newSpaceEndHeapIndex);
}

bool Allocator::isInNewSpace(Object *object) {
	size_t heapIndex = getHeapIndex(object);
	return heapIndex >= newSpaceStartHeapIndex && heapIndex < newSpaceEndHeapIndex;
}

size_t Allocator::getHeapIndex(Object *object) {
	// This conversion is only valid because the heap is an array of bytes.
	return (size_t) ((char *) object->getAddress() - (char *) heap);
}

void Allocator::swapHeaps() {
	size_t tempIndex;

	tempIndex = newSpaceStartHeapIndex;
	newSpaceStartHeapIndex = oldSpaceStartHeapIndex;
	oldSpaceStartHeapIndex = tempIndex;

	tempIndex = newSpaceEndHeapIndex;
	newSpaceEndHeapIndex = oldSpaceEndHeapIndex;
	oldSpaceEndHeapIndex = tempIndex;

	tempIndex = newSpaceRememberedHeapIndex;
	newSpaceRememberedHeapIndex = oldSpaceRememberedHeapIndex;
	oldSpaceRememberedHeapIndex = tempIndex;
}

void Allocator::freeOldSpace() {
	setFree(oldSpaceStartHeapIndex, oldSpaceEndHeapIndex-oldSpaceStartHeapIndex);
}

size_t Allocator::getRegionIndex(Region* region){
	std::vector<heapStats>::iterator it;
	unsigned char* regionHeapAddress = region->getHeapAddress();

	for(it = allHeaps.begin(); it != allHeaps.end(); it++){
		if(regionHeapAddress == (*it).heapStart){
			return (size_t)((unsigned char*)region->getAddress() + ((*it).firstRegion * regionSize));
		}
	}
	return -1;
}

size_t Allocator::getUsedSpace(bool newSpace) {
	size_t i;
	size_t usedSpace = 0;
	if (newSpace) {
		for (i = newSpaceStartHeapIndex; i < newSpaceEndHeapIndex; i++)
			if (isBitSet(i))
				usedSpace++;
	} else {
		for (i = oldSpaceStartHeapIndex; i < oldSpaceEndHeapIndex; i++)
			if (isBitSet(i))
				usedSpace++;
	}
	return (size_t) usedSpace;
}

void Allocator::moveObject(Object *object) {
	if (isInNewSpace(object))
		return;

	size_t size = object->getHeapSize();
	size_t address = (size_t)allocateInNewSpace(size);

	if (address == (size_t)-1) {
		fprintf(stderr, "error moving object (size %zu) with id %d, old space %zu, new space %zu\n", size, object->getID(), getUsedSpace(false), getUsedSpace(true));
		exit(1);
	}
	memcpy((void *) address, (void *) object->getAddress(), size);

	object->updateAddress((void *) address);
	object->setForwarded(true);
}


void Allocator::initializeHeap(size_t heapSize) {
	overallHeapSize = heapSize;
	myHeapBitMap = new char[(size_t)ceil(heapSize/8.0) ];
	heap = (unsigned char*)malloc(heapSize);

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

void Allocator::initializeHeap(size_t heapSize, size_t maxHeapSize) {
}

int Allocator::addRegions(){
	return -1;
}

int Allocator::mergeRegions(){
	return -1;
}

void Allocator::setAllocated(size_t heapIndex, size_t size) {
	size_t i;
	size_t toMark = heapIndex;

	for (i = 0; i < size; i++) {
		setBitUsed(toMark);
		toMark++;
	}
}

// For multiple heaps
void Allocator::setAllocated(unsigned char *heapStart, size_t heapIndex, size_t size) {
	size_t i;
	size_t toMark = 0;
	std::vector<heapStats>::iterator it;
	heapStats currentHeap;

	for(it = allHeaps.begin(); it != allHeaps.end(); ++it){
		currentHeap = *it;
		if(heapStart == currentHeap.heapStart){
			toMark = heapIndex + (currentHeap.firstRegion * regionSize);
		}
	}

	for (i = 0; i < size; i++) {
		setBitUsed(toMark);
		toMark++;
	}
}

void Allocator::setFree(size_t heapIndex, size_t size) {
	size_t i;
	size_t toFree = heapIndex;

	for (i = 0; i < size; i++) {
		setBitUnused(toFree);
		toFree++;
	}
}

void Allocator::setRegionFree(Region* region){
	size_t i;
	size_t toFree = getRegionIndex(region);

	for (i = 0; i < regionSize; i++) {
		setBitUnused(toFree);
		toFree++;
	}
}

size_t Allocator::getHeapSize() {
	return overallHeapSize;
}

size_t Allocator::getRegionSize() {
	// this method can be generalized/overridden to support an arbitrary number of regions.
	return regionSize;
	//return isSplitHeap ? overallHeapSize / 2 : overallHeapSize;
}

std::vector<Region*> Allocator::getRegions() {
	return balancedRegions;
}

std::vector<unsigned int> Allocator::getEdenRegions() {
	return edenRegions;
}

unsigned int Allocator::getNextFreeRegionID() {
	unsigned int id = freeRegions.front();
	freeRegions.erase(freeRegions.begin());
	return id;
}

std::vector<unsigned int> Allocator::getFreeRegions() {
	return freeRegions;
}

// Updated for multiple heaps
unsigned int Allocator::getObjectRegion(Object* object) {
	void *objectAddress = object->getAddress();

	std::vector<heapStats>::iterator it;
	heapStats currentHeap;

	for(it = allHeaps.begin(); it != allHeaps.end(); ++it){
		currentHeap = *it;
		if(objectAddress >= currentHeap.heapStart && objectAddress < currentHeap.heapEnd){
			return (unsigned int)(((size_t)objectAddress - (size_t)currentHeap.heapStart)/regionSize + currentHeap.firstRegion);
		}
	}

	fprintf(balancedLogFile, "Unable to getOBjectRegion for address: %p. Simulation is aborting\n", objectAddress);
	fflush(balancedLogFile);
	fprintf(stderr, "ERROR! Unable to getOBjectRegion. Aborting Simulation...\n");
	fflush(stderr);
	//exit(1);
	throw; //With throw instead of exit: Stack is available for debugging!
}

// Updated for multiple heaps
unsigned int Allocator::getObjectRegionByRawObject(void* objectAddress) {
	std::vector<heapStats>::iterator it;
	heapStats currentHeap;

	for(it = allHeaps.begin(); it != allHeaps.end(); ++it){
		currentHeap = *it;
		if(objectAddress >= currentHeap.heapStart && objectAddress < currentHeap.heapEnd){
			return (unsigned int)(((size_t)objectAddress - (size_t)currentHeap.heapStart)/regionSize + currentHeap.firstRegion);
		}
	}

	fprintf(balancedLogFile, "Unable to getOBjectRegion for address: %p. Simulation is aborting\n", objectAddress);
	fflush(balancedLogFile);
	fprintf(stderr, "ERROR! Unable to getOBjectRegion. Aborting Simulation...\n");
	fflush(stderr);
	//exit(1);
	throw; //With throw instead of exit: Stack is available for debugging!
}


void Allocator::addNewFreeRegion(unsigned int regionID){
	freeRegions.push_back(regionID);
}

void Allocator::removeEdenRegion(unsigned int regionID){
	unsigned int i;
	for (i = 0; i < edenRegions.size(); i++) {
		if (edenRegions[i] == regionID) {
			edenRegions.erase(edenRegions.begin() + i);
			return;
		}
	}
}

void Allocator::printMap() {
	fprintf(heapMap, "%7d", gLineInTrace);

	size_t i;
	for (i = 0; i < overallHeapSize; i++) {
		if (isBitSet(i) == 1) {
			fprintf(heapMap, "X");
		} else {
			fprintf(heapMap, "_");
		}
	}

	fprintf(heapMap, "\n");
}

/*inline*/ bool Allocator::isBitSet(size_t heapIndex) {
	size_t byteNR = heapIndex>>3;
	size_t bit = 7 - heapIndex % 8;
	return ((myHeapBitMap[byteNR] & (1 << bit))>0)?true:false;
}

void Allocator::setBitUsed(size_t heapIndex) {
	if (heapIndex > (size_t) overallHeapSize) {
		fprintf(stderr,
				"ERROR(Line %d): setBitUsed request to illegal slot %zu\n",
				gLineInTrace, heapIndex);
		//exit(1);
		throw; //With throw instead of exit: Stack is available for debugging!
	}

	size_t byte = heapIndex / 8;
	size_t bit = 7 - heapIndex % 8;

	myHeapBitMap[byte] = myHeapBitMap[byte] | 1 << bit;
}

void Allocator::setBitUnused(size_t heapIndex) {
	if (heapIndex > (size_t) overallHeapSize) {
		fprintf(stderr, "add %zu heap %zu\n", heapIndex, overallHeapSize);
		fprintf(stderr, "ERROR: setBitUnused request to illegal slot\n");
	}

	size_t byte = heapIndex / 8;
	size_t bit = 7 - heapIndex % 8;

	myHeapBitMap[byte] = myHeapBitMap[byte] & ~(1 << bit);
}

void Allocator::printStats() {
	if (DEBUG_MODE && WRITE_HEAPMAP) {
		printMap();
	}

	size_t bytesAllocated = overallHeapSize - getFreeSize();

	fprintf(allocLog, "%7d: alloc: %zu obj: %7d\n", gLineInTrace,
			bytesAllocated, statLiveObjects);
}

void Allocator::resetRememberedAllocationSearchPoint() {
	newSpaceRememberedHeapIndex = newSpaceStartHeapIndex;
	oldSpaceRememberedHeapIndex = oldSpaceStartHeapIndex;
}

bool Allocator::isRealAllocator() {
	return false;
}

void Allocator::freeAllSectors() {
	size_t i;
	for (i = 0; i < overallHeapSize; i++) {
		setBitUnused(i);
	}
}

void Allocator::gcFree(Object* object) {
	size_t size = object->getHeapSize();
	size_t heapIndex = getHeapIndex(object);

	setFree(heapIndex, size);
	statLiveObjects--;
}

void Allocator::printStats(long trigReason) {
}

Allocator::~Allocator() {
	//delete[] myHeapBitMap;
}

}
