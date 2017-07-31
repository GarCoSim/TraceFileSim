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

extern TRACE_FILE_LINE_SIZE gLineInTrace;
extern FILE* balancedLogFile;

using namespace std;

namespace traceFileSimulator {

/** Sets fields based on whether the heap is being split in two or not.
 *
 * @deprecated
 * keep  Allocator::setHalfHeapSize(bool) for now,
 * replace it eventually with  Allocator::setNumberOfRegionsHeap(int)
 *
 * @param value true for split heaps, false for all other heaps.
 */
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

/** Sets fields based on whether the heap is a single heap,
 * split heap, or region based heap.
 *
 * @param value 2 for split heap, 1 for whole heap, 0 for regions
 */
void Allocator::setNumberOfRegionsHeap(size_t value) {

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
		size_t i, currentNumberOfRegions, currentMaximumRegions;
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
		maximumMerges = (size_t)ceil(log2(maximumRegionSize/regionSize));
		while(numberOfRegions%(int)pow(2, maximumMerges)){
			numberOfRegions += numberOfRegions;
		}

		overallHeapSize = numberOfRegions * regionSize;
		maxNumberOfEdenRegions = (size_t)(ceil((EDENREGIONS * (double)numberOfRegions)/100));

		// Create heap
		heap = (unsigned char*)malloc(overallHeapSize);
		currentHeap.heapStart = heap;
		currentHeap.heapEnd = heap+overallHeapSize;
		currentHeap.firstRegion = 0;
		allHeaps.push_back(currentHeap);

		// Create the bitmap. Use parens for array created with new.
		myHeapBitMap = new char[(size_t)ceil(overallHeapSize/8.0)]();

		//initialize regions
		Region* balancedRegion;
		size_t currentAddress = 0;

		for (i = 0; i < numberOfRegions; i++) {
			balancedRegion = new Region ((void*)currentAddress, regionSize, heap);

			balancedRegions.push_back(balancedRegion);
			freeRegions.push_back(i);

			currentAddress = currentAddress + regionSize;
		}

		fprintf(stderr, "Heap Size %zd\n", overallHeapSize);
		fprintf(stderr, "Region Size %zu\n", regionSize);
		fprintf(stderr, "Number of Regions %zu\n", numberOfRegions);
		fprintf(balancedLogFile, "Region Statistics:\n");
		fprintf(balancedLogFile, "Heap Size = %zu\n", overallHeapSize);
		fprintf(balancedLogFile, "Region Size = %zu\n", regionSize);
		fprintf(balancedLogFile, "Number of Regions = %zu\n", numberOfRegions);
		fprintf(balancedLogFile, "Maximum number of Eden Regions = %zu\n", maxNumberOfEdenRegions);
		fprintf(balancedLogFile, "Maximum number of Merges = %zu \n\n\n", maximumMerges);
		fflush(balancedLogFile);
	}
}

/** Empty constructor.
 *
 */
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

/** Gets a character pointer to the heap.
 *
 * @return Pointer to the first byte in the heap.
 */
unsigned char *Allocator::getHeap () {
	return heap;
}

/** Gets the number of bytes between the first set byte and the given address.
 *
 * @param start Address from which to start search.
 * @return Address of first set byte.
 */
size_t Allocator::getSpaceToNextObject(size_t start){
	for(size_t i=start; i<getRegionSize(); i++){
		if(isBitSet(i)){
			return i-start;
		}
	}
	ERRMSG("Object found in getNextObjectAddress(), but here it doesn't exist.\n");
	exit (1);
}

/** Gets a pointer to the first object after the given address.
 *
 * @param start Address from which to start search.
 * @return Address of first object.
 */
unsigned char *Allocator::getNextObjectAddress(size_t start){
	for(size_t i=start; i<getRegionSize(); i++){
		if(isBitSet(i)){
			return &heap[i];
		}
	}
	return NULL;
}

/** Attempts to allocate object of given size to the old space.
 * Calls the Allocator::allocate(size_t, size_t, size_t) method.
 *
 * @param size Size in bytes of the object to be allocated.
 * @return return value of Allocator::allocate(size_t, size_t, size_t).
 */
void *Allocator::gcAllocate(size_t size) {
	return allocate(size, oldSpaceStartHeapIndex, oldSpaceEndHeapIndex);
}

/** Attempts to allocate object of given size to the new space.
 * Calls the Allocator::allocate(size_t, size_t, size_t) method.
 *
 * @param size Size in bytes of the object to be allocated.
 * @return return value of Allocator::allocate(size_t, size_t, size_t).
 */
void *Allocator::allocateInNewSpace(size_t size) {
	return allocate(size, newSpaceStartHeapIndex, newSpaceEndHeapIndex);
}

/** Checks if an object is in the new space.
 *
 * @param object Object to test.
 * @return true iff in the new space.
 */
bool Allocator::isInNewSpace(Object *object) {
	size_t heapIndex = getHeapIndex(object);
	return heapIndex >= newSpaceStartHeapIndex && heapIndex < newSpaceEndHeapIndex;
}

/** Gets the location in the heap that an object is in.
 *
 * @param object Object to check.
 * @return the location in the heap.
 */
size_t Allocator::getHeapIndex(Object *object) {
	// This conversion is only valid because the heap is an array of bytes.
	return (size_t) ((char *) object->getAddress() - (char *) heap);
}

/** Swaps the new and old heaps
 *
 */
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

/** Sets all the space in the old heap as allocatable.
 *
 */
void Allocator::freeOldSpace() {
	setFree(oldSpaceStartHeapIndex, oldSpaceEndHeapIndex-oldSpaceStartHeapIndex);
}

/** Calculates which heap the region is in.
 *
 * @param region Region who's heap index is in question.
 * @return The location in the heap of this region.
 */
Optional<size_t>* Allocator::getRegionIndex(Region* region){
	std::vector<heapStats>::iterator it;
	unsigned char* regionHeapAddress = region->getHeapAddress();

	for(it = allHeaps.begin(); it != allHeaps.end(); it++){
		if(regionHeapAddress == (*it).heapStart){
			return new Optional<size_t>((size_t)((unsigned char*)region->getAddress() + ((*it).firstRegion * regionSize)));
		}
	}
	return new Optional<size_t>();
}

/** Gets the used space for the heap. Get's the new space used space
 * iff given true. Gives the old space used space if given false.
 *
 * @param newSpace Flag for checking the new space.
 * @return The amount of used space in bytes.
 */
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
	return usedSpace;
}

/** Copies the object to the new space.
 *
 * @param object Pointer to object to move.
 */
void Allocator::moveObject(Object *object) {
	if (isInNewSpace(object))
		return;

	size_t size = object->getHeapSize();
	size_t address = (size_t)allocateInNewSpace(size);

	if (address == (size_t)-1) {
		fprintf(stderr, "error moving object (size %zu) with id %d, old space %zu, new space %zu\n", size, object->getID(), getUsedSpace(false), getUsedSpace(true));
		exit(1);
	}
	memcpy((void *) address, object->getAddress(), size);

	object->updateAddress((void *) address);
	object->setForwarded(true);
}

/** Sets the initial state of the heap for a given size.
 * Calls malloc to get the requested number of bytes.
 *
 * @param heapSize Size of the heap to make
 */
void Allocator::initializeHeap(size_t heapSize) {
	overallHeapSize = heapSize;
	//Use parens for array created with new.
	myHeapBitMap = new char[(size_t)ceil(heapSize/8.0) ]();
	heap = (unsigned char*)malloc(heapSize);

	statLiveObjects = 0;
	resetRememberedAllocationSearchPoint();

#if DEBUG_MODE && WRITE_ALLOCATION_INFO
	allocLog = fopen("alloc.log", "w+");
#endif
#if DEBUG_MODE && WRITE_HEAPMAP
	heapMap = fopen("heapmap.log", "w+");
#endif
	fprintf(stderr, "heap size %zd\n", overallHeapSize);
}

/** Virtual method to be implemented by subclassing classes
 *
 * @param heapSize size in bytes
 * @param maxHeapSize max size in bytes
 */
void Allocator::initializeHeap(size_t heapSize, size_t maxHeapSize) {
}

/** Virtual method to be implemented by subclassing classes
 *
 * @return
 */
int Allocator::addRegions(){
	return -1;
}

/** Virtual method to be implemented by subclassing classes
 *
 * @return
 */
int Allocator::mergeRegions(){
	return -1;
}

/** Sets the space at a given start and of given size to an allocated state
 *
 * @param heapIndex Index of the start of space
 * @param size Size of space
 */
void Allocator::setAllocated(size_t heapIndex, size_t size) {
	size_t i;
	size_t toMark = heapIndex;

	for (i = 0; i < size; i++) {
		setBitUsed(toMark);
		toMark++;
	}
}

/** A multi-heap version of Allocator::setAllocated(size_t, size_t)
 *
 * @param heapStart Start address for this heap
 * @param heapIndex Heap index to be set
 * @param size Size of heap index to be set
 */
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

/** Frees the space starting at the given index and of the given size.
 *
 * @param heapIndex Start of heap space to be freed.
 * @param size Size of heap space to be freed.
 */
void Allocator::setFree(size_t heapIndex, size_t size) {
	size_t i;
	size_t toFree = heapIndex;

	for (i = 0; i < size; i++) {
		setBitUnused(toFree);
		toFree++;
	}
}

/** Sets all bytes on the region as unused.
 *
 * @param region Region to be freed
 */
void Allocator::setRegionFree(Region* region) {
    size_t i;
    Optional<size_t> *toFreeWrapper = getRegionIndex(region);
    if (toFreeWrapper->isSet()) {
        size_t toFree = toFreeWrapper->getValue();
        for (i = 0; i < regionSize; i++) {
            setBitUnused(toFree);
            toFree++;
        }
    }
	delete(toFreeWrapper);
}

size_t Allocator::getHeapSize() {
	return overallHeapSize;
}

size_t Allocator::getRegionSize() {
	return regionSize;
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

/** Searches linearly through the heaps to find which heap contains the object.
 * Then calculates which region in that heap contains the object.
 *
 * @param object Object who's region is being looked for.
 * @return ID of the region containing the object.
 */
size_t Allocator::getObjectRegion(Object* object) {
	void *objectAddress = object->getAddress();

	std::vector<heapStats>::iterator it;
	heapStats currentHeap;

	for(it = allHeaps.begin(); it != allHeaps.end(); ++it){
		currentHeap = *it;
		if(objectAddress >= currentHeap.heapStart && objectAddress < currentHeap.heapEnd){
			return (((size_t)objectAddress - (size_t)currentHeap.heapStart)/regionSize + currentHeap.firstRegion);
		}
	}

	fprintf(balancedLogFile, "Unable to getObjectRegion for Object ID: %i and address: %p\nSimulation is aborting\n", object->getID(), objectAddress);
	fflush(balancedLogFile);
	fprintf(stderr, "ERROR! Unable to getObjectRegion. Aborting Simulation...\n");
	fflush(stderr);
	//exit(1);
	throw 19; //With throw instead of exit: Stack is available for debugging!
}

/** Searches linearly through the heaps to find which hea contains the address.
 * Then calculates which region in that heap contains the address.
 *
 * @param objectAddress Address who's region is being looked for.
 * @return ID of the region containing the object.
 */
size_t Allocator::getObjectRegionByRawObject(void* objectAddress) {
	std::vector<heapStats>::iterator it;
	heapStats currentHeap;

	for(it = allHeaps.begin(); it != allHeaps.end(); ++it){
		currentHeap = *it;
		if(objectAddress >= currentHeap.heapStart && objectAddress < currentHeap.heapEnd){
			return (((size_t)objectAddress - (size_t)currentHeap.heapStart)/regionSize + currentHeap.firstRegion);
		}
	}

	fprintf(balancedLogFile, "Unable to getObjectRegion for address: %p. Simulation is aborting\n", objectAddress);
	fflush(balancedLogFile);
	fprintf(stderr, "ERROR! Unable to getObjectRegion. Aborting Simulation...\n");
	fflush(stderr);
	//exit(1);
	throw 19; //With throw instead of exit: Stack is available for debugging!

}

/** Pushes a region to the free region collection.
 *
 * @param regionID ID of the region to be added to the collection.
 */
void Allocator::addNewFreeRegion(size_t regionID){
	freeRegions.push_back(regionID);
}

/** Removes the given region id from the eden region collection.
 *
 * @param regionID ID of region to be removed.
 */
void Allocator::removeEdenRegion(size_t regionID){
	unsigned int i;
	for (i = 0; i < edenRegions.size(); i++) {
		if (edenRegions[i] == regionID) {
			edenRegions.erase(edenRegions.begin() + i);
			return;
		}
	}
}

/** Prints the bitmap used for keeping track of which areas of memory
 * are set in the heap to a log file.
 *
 */
void Allocator::printMap() {
	fprintf(heapMap, "" TRACE_FILE_LINE_FORMAT "", gLineInTrace);

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

/** Checks the bitmap to see if the heap index is used or not.
 *
 * @param heapIndex Heap location to be checked.
 * @return true iff used bit is set.
 */
/*inline*/ bool Allocator::isBitSet(size_t heapIndex) {
	size_t byteNR = heapIndex>>3;
	size_t bit = 7 - heapIndex % 8;
	return ((myHeapBitMap[byteNR] & (1 << bit)) > 0);
}

/** Sets the bit in the bitmap of the heap index to used.
 *
 * @param heapIndex Heap location to be set.
 */
void Allocator::setBitUsed(size_t heapIndex) {
	if (heapIndex > (size_t) overallHeapSize) {
		fprintf(stderr,
				"ERROR(Line " TRACE_FILE_LINE_FORMAT "): setBitUsed request to illegal slot %zu\n",
				gLineInTrace, heapIndex);
		//exit(1);
		throw 19; //With throw instead of exit: Stack is available for debugging!
	}

	size_t byte = heapIndex / 8;
	size_t bit = 7 - heapIndex % 8;

	myHeapBitMap[byte] = myHeapBitMap[byte] | (char)(1 << bit);
}

/** Sets the bit in the bitmap of the heap index to unused.
 *
 * @param heapIndex Heap location to be set.
 */
void Allocator::setBitUnused(size_t heapIndex) {
	if (heapIndex > (size_t) overallHeapSize) {
		fprintf(stderr, "add %zu heap %zu\n", heapIndex, overallHeapSize);
		fprintf(stderr, "ERROR: setBitUnused request to illegal slot\n");
	}

	size_t byte = heapIndex / 8;
	size_t bit = 7 - heapIndex % 8;

	myHeapBitMap[byte] = myHeapBitMap[byte] & (char)(~(1 << bit));
}

/** Prints information about the heap to a log file.
 *
 */
void Allocator::printStats() {
#if DEBUG_MODE && WRITE_HEAPMAP
	printMap();
#endif

	size_t bytesAllocated = overallHeapSize - getFreeSize();

	fprintf(allocLog, "" TRACE_FILE_LINE_FORMAT ": alloc: %zu obj: %7d\n", gLineInTrace,
			bytesAllocated, statLiveObjects);
}

/** Sets the remembered heap indexes to the start heap indexes.
 *
 */
void Allocator::resetRememberedAllocationSearchPoint() {
	newSpaceRememberedHeapIndex = newSpaceStartHeapIndex;
	oldSpaceRememberedHeapIndex = oldSpaceStartHeapIndex;
}

/** Method called to determine if this is an implementing subclass.
 *
 * @return true iff this is an inplementing subclass.
 */
bool Allocator::isRealAllocator() {
	return false;
}

/** Sets the object as free.
 *
 * @param object Object to be freed
 */
void Allocator::gcFree(Object* object) {
	size_t size = object->getHeapSize();
	size_t heapIndex = getHeapIndex(object);

	setFree(heapIndex, size);
	statLiveObjects--;
}

/** Virtual method to be implemented by subclass.
 *
 * @param size size of object
 * @param lower start of allocation space
 * @param upper end of allocation space
 * @return Pointer to object location. NULL on failure.
 */
void *Allocator::allocate(size_t size, size_t lower, size_t upper) {
	return NULL;
}

void Allocator::printStats(long trigReason) {
}

Allocator::~Allocator() {
	//delete[] myHeapBitMap;
}

}
