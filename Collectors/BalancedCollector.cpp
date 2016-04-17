/*
 * TraversalCollector.hpp
 *
 *  Created on: 2015-10-27
 *      Author: GarCoSim
 */

#include "BalancedCollector.hpp"
#define MAXAGE 23 //The biggest age that will be considered
#define MAXAGEP 10 //Probability of MAXAGE to be chosen in %
#define COLLECTIONSETSIZE 0.5 //Absolute maximum size of collection set. default is 0.5 which means 50% of all regions

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


	allRegions = myAllocator->getRegions();
	emptyHelpers();
	preCollect();


	buildCollectionSet();
	copy();
	updateRemsetPointers();
	myPrintStatsQueue = myUpdatePointerQueue;

	//fprintf(balancedLogFile, "\nObjects before updatePointers: \n");
	//printObjects();
	updatePointers();
	//fprintf(balancedLogFile, "\nObjects after updatePointers: \n");
	//printObjects();

	reOrganizeRegions();
	postCollect();
	printStats();
	stop = clock();
	elapsed_secs = double(stop - start)/CLOCKS_PER_SEC;
	fprintf(stderr, " took %0.3fs\n", elapsed_secs);
}


void BalancedCollector::emptyHelpers() {
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

	statFreedDuringThisGC = 0;
}

void BalancedCollector::preCollect() {
	start = clock();
	statFreedDuringThisGC = 0;
	statGcNumber++;
}


void BalancedCollector::buildCollectionSet() {
	fprintf(balancedLogFile, "\n\nBuilding collection set:\n");

	int setSize = (int)(COLLECTIONSETSIZE*allRegions.size());
	
	myCollectionSet.clear();
	myCollectionSet.resize(allRegions.size(), 0);
	int regionsInSet = 0;
	
	std::vector<unsigned int> edenRegions = myAllocator->getEdenRegions();
	unsigned int i, j;
	
	//For now: Add all and only the Eden Regions
	for (i = 0; i < allRegions.size(); i++) {
		for (j = 0; j < edenRegions.size(); j++) {
			if (i == edenRegions[j]) {
				regionsInSet++;
				myCollectionSet[i] = 1;
				fprintf(balancedLogFile, "Added eden region %i to collection set\n", i);
			}
		}
	}

	Region* currentRegion;
	int regionAge;
	float probability;
	int dice;

	//linear function passing two points (0,1) (age 0 always selected) and (MAXAGE, MAXAGEP) 
	//there is maximum age which can also be picked with some non-0 probability
	for (i = 0; (i < allRegions.size() && regionsInSet<=setSize); i++) {
		if (myCollectionSet[i]==0) {
			currentRegion = allRegions[i];
			regionAge = currentRegion->getAge();
			probability = (100-MAXAGEP)/(0-MAXAGE)*regionAge+100;
			dice = rand()%100+1;
			if (dice<=probability) {
				myCollectionSet[i] = 1;
				fprintf(balancedLogFile, "Added region %i of age %i to collection set\n", i, regionAge);
				regionsInSet++;
			}
		}
	}
}


void BalancedCollector::copy() {
	fprintf(balancedLogFile, "Start copying\n");
	
	getRootObjects();
	copyRootObjects();
	getRemsetObjects();
	copyRootObjects();

	
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
		//fprintf(balancedLogFile, "Found %zu root objects for Thread %i\n", roots.size(), i);
		
		for (j = 0; j < roots.size(); j++) {
			currentObj = roots[j];
			if (currentObj && !currentObj->getVisited()) {
				currentObj->setVisited(true);
				//fprintf(balancedLogFile, "Current object: ID = %i. Child: ID = %i. Address = %ld\n", currentObj->getID(), currentObj->getReferenceTo(0)->getID(), (long)currentObj->getReferenceTo(0)->getAddress());
				objectRegion = myAllocator->getObjectRegion(currentObj);
				//fprintf(balancedLogFile, " Appropriate object Region: %u\n", objectRegion);

				if (myCollectionSet[objectRegion] == 1) { //Object belongs to Collection-set-Region
					myQueue.push(currentObj);
					
				}
			}
		}
	}
	myUpdatePointerQueue = myQueue;
	fprintf(balancedLogFile, "Getting Root Objects done. Added %zu objects to the queue.\n\n", myQueue.size());
}

void BalancedCollector::copyRootObjects() {
	fprintf(balancedLogFile, "Copy Objects in queue\n\n");
	
	int i;
	unsigned int childRegion;
	Object* currentObj;
	Object* child;

	//breadth first through the tree using a queue
	while (!myQueue.empty()) {
		currentObj = myQueue.front();
		myQueue.pop();
		int children = currentObj->getPointersMax();

		//fprintf(balancedLogFile, "Copying object:\n");
		printStuff(currentObj);
		copyAndForwardObject(currentObj);
		//fprintf(balancedLogFile, "After Copying object:\n");
		printStuff(currentObj);
		//fprintf(balancedLogFile, "\n");
				

		//currentObj->setAge(currentObj->getAge() + 1); //Delete this?!
		//fprintf(balancedLogFile, "Getting children for object: %i\n", currentObj->getID());
		for (i = 0; i < children; i++) {
			child = currentObj->getReferenceTo(i);
			if (child && !child->getVisited()) {
				//fprintf(balancedLogFile, "Found child: %i. Address: %ld\n", child->getID(), (long)child->getAddress());
				childRegion =  myAllocator->getObjectRegion(child);
				if (myCollectionSet[childRegion] == 1) {
				child->setVisited(true);
				myQueue.push(child);
				myUpdatePointerQueue.push(child);
				}
			}
		}
	}
	
	fprintf(balancedLogFile, "\nCopying enqueued Objects done. Copied %u objects.\n\n", statFreedDuringThisGC);
}

void BalancedCollector::getRemsetObjects() {
	fprintf(balancedLogFile, "Get Remset Objects\n");

	Object* currentObj;
	std::set<void*> currentRemset;
	std::set<void*>::iterator remsetIterator;
	int j;
	int children;
	unsigned int parentRegion, childRegion, i;
	Object* child;
	void* remsetPointer; 

	for (i = 0; i < myCollectionSet.size(); i++) {
		if (myCollectionSet[i] == 1) {
			currentRemset = allRegions[i]->getRemset();
			for (remsetIterator = currentRemset.begin(); remsetIterator != currentRemset.end(); ++remsetIterator) {
				
				remsetPointer = *remsetIterator;
				RawObject *rawObject = (RawObject *) remsetPointer; 
				currentObj = rawObject->associatedObject;
				myUpdatePointerQueue.push(currentObj);

					parentRegion =  myAllocator->getObjectRegion(currentObj);
					if (myCollectionSet[parentRegion] == 0) {
						children = currentObj->getPointersMax();
						for (j = 0; j < children; j++) {
							child = currentObj->getReferenceTo(j);
							if (child && !child->getVisited()) {
								child->setVisited(true);
								childRegion =  myAllocator->getObjectRegion(child);

								if (myCollectionSet[childRegion] == 1) {
									myQueue.push(child);
									myUpdatePointerQueue.push(child);

								}
							}
						}
					}
			}
		}
	}
	fprintf(balancedLogFile, "Getting Remset Objects done. Added %zu objects to the queue.\n", myQueue.size());
	fprintf(balancedLogFile, "%zu pointers to be modified.\n\n", myUpdatePointerQueue.size());
}

void BalancedCollector::copyAndForwardObject(Object *obj) {
	unsigned int i, currentCopyToRegionID, objRegionID;
	size_t currentFreeSpace, objectSize;
	void *currentFreeAddress, *addressAfter;
	int objRegionAge;

	objRegionID  = myAllocator->getObjectRegion(obj);
	objRegionAge = allRegions[objRegionID]->getAge();
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

			addressAfter = (void *) obj->getAddress();
			obj->setForwardedPointer(addressAfter);
			statFreedDuringThisGC++;
			//fprintf(balancedLogFile, "Copied object %i from region %u to region %u. Address before: %ld. Address after: %ld\n", obj->getID(), objRegionID, currentCopyToRegionID, (long)addressBefore, (long)addressAfter);
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
				//fprintf(balancedLogFile, "\nGetting new copyToRegion with id: %u\n", currentCopyToRegionID);
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

				addressAfter = (void *) obj->getAddress();
				obj->setForwardedPointer(addressAfter);
				statFreedDuringThisGC++;
				//fprintf(balancedLogFile, "Copied object %i from region %u to region %u. Address before: %ld. Address after: %ld\n", obj->getID(), objRegionID, currentCopyToRegionID, (long)addressBefore, (long)addressAfter);
				return;
			}
			else if (objectSize > currentFreeSpace) {
				fprintf(stderr, "Allocation for arraylets not yet implemented!\n");
			}
		}

		fprintf(stderr, "No more Regions for copying available!\n");
}

void BalancedCollector::updateRemsetPointers() {
	fprintf(balancedLogFile, "Update remset pointers\n");

	unsigned int i;
	std::set<void*> currentRemset;
	std::set<void*>::iterator remsetIterator;
	void* forwardPointer;
	void* remsetPointer;
	unsigned int objectRegion;

	for (i = 0; i < allRegions.size(); i++) {
		if (myCollectionSet[i] == 0) { //Not from collection set
			currentRemset = allRegions[i]->getRemset();
			//fprintf(balancedLogFile, "Region = %u. Remset size = %zu\n", i, currentRemset.size());
			for (remsetIterator = currentRemset.begin(); remsetIterator != currentRemset.end(); ++remsetIterator) {
				
				remsetPointer = *remsetIterator;
				//fprintf(balancedLogFile, "remsetPointer = %ld\n", (long)remsetPointer);

			    RawObject *rawObject = (RawObject *) remsetPointer;
				Object *obj = rawObject->associatedObject;

			    if (obj) {
			    	forwardPointer = obj->getForwardedPointer();
					//fprintf(balancedLogFile, "forwardPointer for object %i = %zu\n", obj->getID(), (size_t)forwardPointer);
			    	//This one is not working!
			    	objectRegion = myAllocator->getObjectRegionByRawObject(forwardPointer);
			    	//fprintf(balancedLogFile, "objectRegion = %u\n", objectRegion);
			    	
			    	if (myCollectionSet[objectRegion] == 1) { //pointing to region from collection set? -> Delete pointer
			    		allRegions[i]->eraseObjectReferenceWithoutCheck(remsetPointer);
			    	}
			    	else {				    	//Not pointing to region from collecton set. Just erase old entry and insert new entry
				    	allRegions[i]->eraseObjectReferenceWithoutCheck(remsetPointer);
				    	//insert new value here
				    	allRegions[i]->insertObjectReference(forwardPointer);
			    	}
			    }			   
			}
			/*
			fprintf(balancedLogFile, "Remset after updated:\n");
			currentRemset = allRegions[i]->getRemset();
			for (remsetIterator = currentRemset.begin(); remsetIterator != currentRemset.end(); ++remsetIterator) {
				remsetPointer = *remsetIterator;
				fprintf(balancedLogFile, "remsetPointer = %ld\n", (long)remsetPointer);
			}
			*/
		}
	}
		

	fprintf(balancedLogFile, "Updating remset pointers done\n");
}

void BalancedCollector::updatePointers() {
	fprintf(balancedLogFile, "Update  pointers\n");

	int j;
	Object* currentObj;
	Object* child;
	int children;
	int parentRegion, childRegion;

	while (!myUpdatePointerQueue.empty()) {
		currentObj = myUpdatePointerQueue.front();
		myUpdatePointerQueue.pop();
		children = currentObj->getPointersMax();
		for (j = 0; j < children; j++) {
			child = currentObj->getReferenceTo(j);
			if (child) {
				currentObj->setRawPointerAddress(j, child->getForwardedPointer());
				childRegion =  myAllocator->getObjectRegion(child);
				parentRegion =  myAllocator->getObjectRegion(currentObj);
				if (childRegion!=parentRegion && myCollectionSet[parentRegion] == 0) {
					allRegions[childRegion]->insertObjectReference(currentObj->getAddress());
				}
			}
		}

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

void BalancedCollector::reOrganizeRegions(){
	unsigned int i;
	Region* currentRegion;

	//Clean up region, free regions, and eden regions
	for (i = 0; i < myCollectionSet.size(); i++) {
		if (myCollectionSet[i] == 1) {
			currentRegion = allRegions[i];
			currentRegion->reset();
			myAllocator->addNewFreeRegion(i);
			myAllocator->removeEdenRegion(i);
		}
	}
}

void BalancedCollector::printObjects(){
	Object* currentObj;
	queue<Object *> myPrintStatsQueueSave = myPrintStatsQueue; //Do this to keep elements in the myPrintStatsQueue for next call
	while(!myPrintStatsQueue.empty()) {
		currentObj = myPrintStatsQueue.front();
		myPrintStatsQueue.pop();
		printStuff(currentObj);
	}
	myPrintStatsQueue = myPrintStatsQueueSave;
}

void BalancedCollector::printStuff(Object* obj){
	int k;
	Object* child;
	void* rawChildAddress;
	fprintf(balancedLogFile, "ID = %i. Address = %ld.    ", obj->getID(), (long)obj->getAddress());
	int children = obj->getPointersMax();
	for (k = 0; k < children; k++) {
		child = obj->getReferenceTo(k);
		rawChildAddress = obj->getRawPointerAddress(k);
		if (child) {
			fprintf(balancedLogFile, "Child %i: ID = %i. Address = %ld.  ", k, child->getID(), (long)rawChildAddress);
		}
	}
	fprintf(balancedLogFile, "\n");
}

void BalancedCollector::printStats() {
	fprintf(balancedLogFile, "Garbage Collection done!\n");
	fprintf(balancedLogFile, "Amount of free regions: %zu. Amount of eden regions: %zu \n\n", myAllocator->getFreeRegions().size(), myAllocator->getEdenRegions().size());
}

BalancedCollector::~BalancedCollector() {
}

}