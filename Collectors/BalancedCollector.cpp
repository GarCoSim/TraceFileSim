/*
 * TraversalCollector.hpp
 *
 *  Created on: 2015-10-27
 *      Author: GarCoSim
 */

#include "BalancedCollector.hpp"
#define MAXAGE 23 //The biggest age that will be considered
#define MAXAGEP 0.1 //Probability of MAXAGE to be chosen
//#define RAND_MAX 1 //for random numbers to be generated

extern int gLineInTrace;
extern FILE* gLogFile;
extern FILE* gDetLog;
extern FILE* balancedLogFile;

extern FILE* gcFile;

extern clock_t start, stop;

namespace traceFileSimulator {

BalancedCollector::BalancedCollector() {
}


void BalancedCollector::initializeHeap() {
	myAllocator->setNumberOfRegionsHeap(0); 
}


void BalancedCollector::collect(int reason) {
	statCollectionReason = reason;
	stop = clock();
	double elapsed_secs = double(stop - start)/CLOCKS_PER_SEC;
	fprintf(stderr, "GC #%d at %0.3fs\n", statGcNumber + 1, elapsed_secs);

	while (!myQueue.empty()) {
		myQueue.pop();
	}
	while (!myUpdatePointerQueue.empty()) {
		myUpdatePointerQueue.pop();
	}
	int i;
	for (i = 0; i < MAXREGIONAGE; i++) {
		copyToRegions[i].clear();
	}

	allRegions = myAllocator->getRegions();

	preCollect();
	buildCollectionSet();
	copy();
	//addCopiedRegionsToFreeList();
	
	stop = clock();
	elapsed_secs = double(stop - start)/CLOCKS_PER_SEC;
	fprintf(stderr, " took %0.3fs\n", elapsed_secs);
}


void BalancedCollector::preCollect() {
	start = clock();
	statFreedDuringThisGC = 0;
	statGcNumber++;
}


void BalancedCollector::buildCollectionSet() {
	fprintf(balancedLogFile, "\n\nBuilding collection set:\n");
	
	myCollectionSet.clear();
	myCollectionSet.resize(myAllocator->getRegions().size(), 0);
	
	std::vector<unsigned int> edenRegions = myAllocator->getEdenRegions();
	unsigned int i, j;
	
	//For now: Add all and only the Eden Regions
	for (i = 0; i < allRegions.size(); i++) {
		for (j = 0; j < edenRegions.size(); j++) {
			if (i == edenRegions[j]) {
				myCollectionSet.at(i) = 1;
				fprintf(balancedLogFile, "Added region %i to collection set\n", i);
			}
		}
		
		/*
		Region* currentRegion = allRegions[i];
		
		//linear function passing two points (0,1) (age 0 always selected) and (MAXAGE, MAXAGEP) 
		//there is maximum age which can also be picked with some non-0 probability
		float mortalityRate;
		int regionAge = currentRegion->getAge();
		float probability = regionAge*(MAXAGEP+1)/MAXAGE+1; 
		if ( rand() < probability ) {
			myCollectionSet.push_back (i);
		}
		*/
	}
}


void BalancedCollector::copy() {
	fprintf(balancedLogFile, "Start copying\n");
	
	getRootObjects();
	copyRootObjects();
	//updatePointers();
	
	//copyRemsetObjects();
	
	fprintf(balancedLogFile, "Copying done\n");
}


void BalancedCollector::getRootObjects() {
	fprintf(balancedLogFile, "Get Root Objects\n");
	Object* currentObj;
	int i;
	unsigned int objectRegion, j;

	// enqueue all roots, copy only objects from collectionSet
	vector<Object*> roots;
	for (i = 0; i < NUM_THREADS; i++) {
		roots = myObjectContainer->getRoots(i);
		fprintf(balancedLogFile, "Found %zu root objects for Thread %i\n", roots.size(), i);
		
		for (j = 0; j < roots.size(); j++) {
			currentObj = roots[j];
			//fprintf(balancedLogFile, "Current root object from outside: %i. ID: %i\n", j, currentObj->getID());
			if (currentObj && !currentObj->getVisited()) {
				currentObj->setVisited(true);
				//fprintf(balancedLogFile, "Current object: ID = %i. ", currentObj->getID());
				objectRegion = myAllocator->getObjectRegion(currentObj);
				//fprintf(balancedLogFile, " Appropriate object Region: %u\n", objectRegion);

				if (myCollectionSet[objectRegion] == 1) { //Object belongs to Collection-set-Region
					myQueue.push(currentObj);
					//regionAge = myAllocator->getRegions().at(objectRegion)->getAge();
					//fprintf(balancedLogFile, "Current root object from inside: %i\n", j);
					//fprintf(balancedLogFile, "Thread %i. Added object %i from region %u to queue.\n", i, currentObj->getID(), objectRegion);
					//copyObject(currentObj, regionAge);
					
				}
			}
		}
	}
	myUpdatePointerQueue = myQueue;
	fprintf(balancedLogFile, "Getting Root Objects done. Added %zu objects to the queue.\n\n", myQueue.size());
}

void BalancedCollector::copyRootObjects() {
	fprintf(balancedLogFile, "Copy Root Objects\n");
	
	int i;
	unsigned int childRegion;
	Object* currentObj;
	Object* child;

	//breadth first through the tree using a queue
	while (!myQueue.empty()) {
		currentObj = myQueue.front();
		myQueue.pop();
		int children = currentObj->getPointersMax();
		copyAndForwardObject(currentObj);

		//currentObj->setAge(currentObj->getAge() + 1); //Delete this?!

		for (i = 0; i < children; i++) {
			child = currentObj->getReferenceTo(i);
			if (child) {
				childRegion =  myAllocator->getObjectRegion(child);
				if (!child->getVisited() && myCollectionSet[childRegion] == 1) {
				child->setVisited(true);
				myQueue.push(child);
				myUpdatePointerQueue.push(child);
				}
			}
		}
	}
	
	fprintf(balancedLogFile, "Copying Root Objects done\n\n");
}

void BalancedCollector::copyAndForwardObject(Object *obj) {
	unsigned int i, currentCopyToRegionID, objRegionID;
	size_t currentFreeSpace, objectSize;
	void *currentFreeAddress, *addressBefore, *addressAfter;
	int objRegionAge;

	objRegionID  = myAllocator->getObjectRegion(obj);
	objRegionAge = allRegions[objRegionID]->getAge();
	addressBefore = (void*) obj->getAddress();
	objectSize = obj->getHeapSize();

	//Find a region to copy to
	for (i = 0; i < copyToRegions[objRegionAge].size(); i++) {
		currentCopyToRegionID = copyToRegions[objRegionAge][i];
		currentFreeSpace = allRegions[currentCopyToRegionID]->getCurrFree();
		if (objectSize <= currentFreeSpace) {
			currentFreeAddress = allRegions[currentCopyToRegionID]->getCurrFreeAddr();	
			allRegions[currentCopyToRegionID]->setCurrFreeAddr((void*)((long)currentFreeAddress+(long)objectSize));
			allRegions[currentCopyToRegionID]->setCurrFree(currentFreeSpace-objectSize);
			allRegions[currentCopyToRegionID]->incrementObjectCount();
			
			myAllocator->setAllocated((long)currentFreeAddress, objectSize);

			void * addressHeap;
			unsigned char *heap = myAllocator->getHeap();
			addressHeap = &heap[(long)currentFreeAddress];

			memcpy((void *) addressHeap, (void *) obj->getAddress(), objectSize);

			obj->updateAddress((void *) addressHeap);
			obj->setForwarded(true);

			addressAfter = (void *) obj->getAddress();
			addForwardingEntry(addressBefore, addressAfter);
			fprintf(balancedLogFile, "Copied object %i from region %u to region %u. Address before: %ld. Address after: %ld\n", obj->getID(), objRegionID, currentCopyToRegionID, (long)addressBefore, (long)addressAfter);
			return;
		}
	}

	//No copyToRegion found, so add a new one
	if ( myAllocator->getFreeRegions().size() > 0) {
			currentCopyToRegionID = myAllocator->getNextFreeRegionID();
			currentFreeSpace = allRegions[currentCopyToRegionID]->getCurrFree();

			copyToRegions[objRegionAge].push_back(currentCopyToRegionID);
			allRegions[currentCopyToRegionID]->setAge(objRegionAge+1);

			if (objectSize <= currentFreeSpace) {
				fprintf(balancedLogFile, "\nGetting new copyToRegion with id: %u\n", currentCopyToRegionID);
				currentFreeAddress = allRegions[currentCopyToRegionID]->getCurrFreeAddr();
				allRegions[currentCopyToRegionID]->setCurrFreeAddr((void*)((long)currentFreeAddress+(long)objectSize));
				allRegions[currentCopyToRegionID]->setCurrFree(currentFreeSpace-objectSize);
				allRegions[currentCopyToRegionID]->incrementObjectCount();
				
				
				myAllocator->setAllocated((long)currentFreeAddress, objectSize);

				void * addressHeap;
				unsigned char *heap = myAllocator->getHeap();
				addressHeap = &heap[(long)currentFreeAddress];

				memcpy((void *) addressHeap, (void *) obj->getAddress(), objectSize);

				obj->updateAddress((void *) addressHeap);
				obj->setForwarded(true);

				addressAfter = (void *) obj->getAddress();
				addForwardingEntry(addressBefore, addressAfter);
				fprintf(balancedLogFile, "Copied object %i from region %u to region %u. Address before: %ld. Address after: %ld\n", obj->getID(), objRegionID, currentCopyToRegionID, (long)addressBefore, (long)addressAfter);
				return;
			}
			else if (objectSize > currentFreeSpace) {
				fprintf(stderr, "Allocation for arraylets not yet implemented!\n");
			}
		}

		fprintf(stderr, "No more Regions for copying available!\n");
}

void BalancedCollector::updatePointers() {
	fprintf(balancedLogFile, "Update pointers\n");

	Object* currentObj;

	while (!myUpdatePointerQueue.empty()) {
		currentObj = myUpdatePointerQueue.front();
		myUpdatePointerQueue.pop();

		currentObj->setVisited(false);
	}

	fprintf(balancedLogFile, "Updating pointers done\n");
}


void BalancedCollector::copyRemsetObjects() {
	//unsigned int i;
	//Object* currentObj;
	//Region* currentRegion;

	/*
	//breadth first through the tree using a queue
	while (!myQueue.empty()) {
		currentObj = myQueue.front();
		myQueue.pop();
		
		//get actual Obejct Address
		size_t objectAddress = myAllocator->getHeapIndex(currentObj);
		//get regionAddress for the object
		size_t regionAddressForObject;
		
		//Check if Object belongs to a Region from collectionSet
		//size_t regionAddress = currentObj->getRegion();
		void *blaAdress;
		for (i = 0; i < collectionSet.size(); i++) {
			currentRegion = collectionSet[i];
			regionAddress = currentRegion->getAddress();
			if ((size_t)blaAdress == regionAddress) {
				fprintf(stderr, "This is some shit\n");
			}
		}
	}
	*/	
	
	
	//unsigned char *blaHeap = myAllocator->getHeap();
	//size_t regionBla = object->getRegion((size_t)&blaHeap[0], myAllocator->getRegionSize());
		
		/*
		//Stuff from TraversalCollector
		int kids = currentObj->getPointersMax();
		copyAndForwardObject(currentObj);
		currentObj->setAge(currentObj->getAge() + 1);
		for (i = 0; i < kids; i++) {
			child = currentObj->getReferenceTo(i);
			//no matter if the child was processed before or not, add it to the rem set.
			if(child && child->getGeneration() < currentObj->getGeneration()){
				myMemManager->requestRemSetAdd(child);
			}
			if (child && !child->getVisited() && child->getGeneration() <= myGeneration) {
				child->setVisited(true);

				myQueue.push(child);
			}
		}
		*/
	
}

BalancedCollector::~BalancedCollector() {
}

}