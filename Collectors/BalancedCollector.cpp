/*
 * TraversalCollector.hpp
 *
 *  Created on: 2015-10-27
 *      Author: GarCoSim
 */
#include <iostream>
#include "BalancedCollector.hpp"
#define MAXAGEP 10 //Probability of MAXAGE to be chosen in %
#define COLLECTIONSETSIZE 0.5 //Absolute maximum size of collection set. default is 0.5 which means 50% of all regions

extern TRACE_FILE_LINE_SIZE gLineInTrace;
extern FILE* gLogFile;
extern FILE* gDetLog;
extern FILE* balancedLogFile;


extern clock_t start, stop;

namespace traceFileSimulator {

BalancedCollector::BalancedCollector() {
}

/** Sets the number of regions on the heap to 0
 *
 */
void BalancedCollector::initializeHeap() {
	myAllocator->setNumberOfRegionsHeap(0);
}

/** Creates a collection set and then performs copy forward collection on
 * that set into the free regions. For more details see IBMs documentation
 * on Balanced Collection
 *
 * @param reason If reason is reasonForced (see defines.hpp) then call
 *          BalancedCollector::buildFinalCollectionSet(). Otherwise call
 *          BalancedCollector::buildCollectionSet()
 */
void BalancedCollector::collect(int reason) {
	int returnVal;
	statCollectionReason = reason;
	stop = clock();
	double elapsed_secs = double(stop - start)/CLOCKS_PER_SEC;
	fprintf(stderr, "GC #%zu at %0.3fs\n", statGcNumber + 1, elapsed_secs);
	fprintf(balancedLogFile, "Starting GC #%zu at %0.3fs\n", statGcNumber + 1, elapsed_secs);
	emptyHelpers();
	preCollect();

	allRegions = myAllocator->getRegions();
	calculateDeadSpace();
	if(reason == (int)reasonForced){
		buildFinalCollectionSet();
	}
	else{
		buildCollectionSet();
	}

	returnVal = copy();
	if (returnVal == -1) {
		//No more regions for copying available, end simulation
		fprintf(balancedLogFile, "No more free Regions available to copy Objects!\n");
		stop = clock();
		elapsed_secs = double(stop - start)/CLOCKS_PER_SEC;
		fprintf(stderr, " took %0.3fs\n. Copying Objects was unsuccessful! Stopping simulation.\n", elapsed_secs);
		fflush(balancedLogFile);
		fflush(stderr);
		exit(1);
	}

	removeObjects();
	updateRemsetPointers();
	updatePointers();

	reOrganizeRegions();

	//Finish up
	statFreedDuringThisGC = totalObjectsInCollectionSet - statCopiedDuringThisGC;
	statFreedObjects += statFreedDuringThisGC;

	postCollect();
	printFinalStats();
	stop = clock();
	elapsed_secs = double(stop - start)/CLOCKS_PER_SEC;
	fprintf(stderr, " took %0.3fs\n", elapsed_secs);
	fflush(balancedLogFile);
}

/** Used for adding spine remsets to regions with spines.
 *
 * @param spine Pointer to the spine object
 */
void BalancedCollector::addObjectToSpineRemset(Object* spine){
	myAllocator->getRegions()[myAllocator->getObjectRegion(spine)]->spineRemset.insert(spine);
}

/** Empties all data structures used to manage a collection.
 *
 */
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
}

/** Sets initial statistical values to defaults.
 *
 */
void BalancedCollector::preCollect() {
	start = clock();
	statCopiedDuringThisGC = 0;
	totalObjectsInCollectionSet = 0;
	statGcNumber++;
}

/** Calculates how much of the heap is being used by dead objects.
 * Prints without returning any values.
 */
void BalancedCollector::calculateDeadSpace(){
	size_t i, j;
	size_t k;
	size_t regionSize, heapPosition, regionEnd;
	size_t heapDeadSpace, heapLiveSpace;
	vector<Object*> roots;
	Object* currentObject;
	RawObject* raw;

	// Calculate the percentage of each region that is occupied by dead objects
	heapDeadSpace = 0;
	heapLiveSpace = 0;
	regionSize = myAllocator->getRegionSize();
	deadSpace.clear();
	deadSpace.resize(allRegions.size());

	// Traverse the rootset
	for(i = 0; i < NUM_THREADS; i++){
		roots = myObjectContainer->getRoots(i);
		for(j = 0; j < roots.size(); j++){
			currentObject = roots[j];
			mark(currentObject); //Mark object and children as visited (alive)
		}
	}

	for(i = 0; i < allRegions.size() ; i++){
		if(!allRegions[i]->getIsLeaf()){
			(deadSpace.at(i)).regionID = i; //Record region ID
			(deadSpace.at(i)).percentDead = regionSize - allRegions[i]->getCurrFree();
			Optional<size_t>* heapPositionWrapper = myAllocator->getRegionIndex(allRegions[i]);
			heapPosition = heapPositionWrapper->getValue();
			delete(heapPositionWrapper);
			regionEnd = (i+1) * regionSize;

			while(heapPosition < regionEnd){ //Search the region for objects
				raw = (RawObject *)myAllocator->getNextObjectAddress(heapPosition);

				if(raw!=NULL){ //If the raw object exists, get associated object
					currentObject = raw->associatedObject;
					heapPosition += myAllocator->getSpaceToNextObject(heapPosition);

					if (currentObject) { //If the object exists
						heapPosition += currentObject->getHeapSize();

						if(currentObject->getVisited()){ //If the object is reachable from the rootset
							heapLiveSpace += currentObject->getHeapSize();
							(deadSpace.at(i)).percentDead -= currentObject->getHeapSize(); //Remove size of live object
							currentObject->setVisited(false); //Reset isVisited
						}
						else if(currentObject->allocationType == allocationTypeDiscontiguousIndexable && currentObject->getID() != -1){ //If dead arraylet spine, mark leaf regions as dead
							for(k = 0; k < currentObject->getPointersMax(); k++){
								size_t leafRegion = myAllocator->getObjectRegion(currentObject->getReferenceTo(k));
								if(allRegions[leafRegion]->getIsLeaf()){
									(deadSpace.at(leafRegion)).percentDead = regionSize;
								}
								else{
									(deadSpace.at(leafRegion)).percentDead -= currentObject->getReferenceTo(k)->getHeapSize();
								}
							}
						}
					} else{
						break;
					}
				} else{
					break;
				}
			}

		}
	}
	for(i = 0; i < allRegions.size(); i++){
		heapDeadSpace += (deadSpace.at(i)).percentDead;
		(deadSpace.at(i)).percentDead == 0 ? 0 : (deadSpace.at(i)).percentDead = 100*(deadSpace.at(i)).percentDead / (regionSize - allRegions[i]->getCurrFree()); //Express as a percent
	}
	heapDeadSpace == 0 ? heapDeadSpace = 0 : heapDeadSpace = 100*heapDeadSpace/(heapDeadSpace + heapLiveSpace);
	fprintf(balancedLogFile, "%zu percent of the occupied heap is dead objects\n", heapDeadSpace);
}

/** Marks an object by setting it's visited status to true and then
 * recursively marks it's children.
 *
 * @param currentObject The object at the root of the object tree to be marked
 */
void BalancedCollector::mark(Object* currentObject){
// Set an object and its children as visited
	size_t numberOfChildren, i;
	size_t numberOfLeaves, j;
	Object* childObject;

	if(!currentObject->getVisited()){
		currentObject->setVisited(true);
		if(currentObject->allocationType == allocationTypeDiscontiguousIndexable && currentObject->getID() != -1){
			numberOfLeaves = currentObject->getPointersMax();
			for(i = 0; i < numberOfLeaves; i++){
				if(currentObject->getReferenceTo(i)){
					currentObject->getReferenceTo(i)->setVisited(true);
					numberOfChildren = currentObject->getReferenceTo(i)->getPointersMax();
					for(j = 0; j < numberOfChildren; j++){
						childObject = currentObject->getReferenceTo(i)->getReferenceTo(j);
						if(childObject){
							mark(childObject);
						}
					}
				}
			}
		}
		else if(currentObject->allocationType == allocationTypeObject){
			numberOfChildren = currentObject->getPointersMax();
			for(i = 0; i < numberOfChildren; i++){
				childObject = currentObject->getReferenceTo(i);
				if(childObject){
					mark(childObject);
				}
			}
		}
	}
}

/** clears the collection set then adds all eden regions and fills
 * remaining space with other regions if collection set is not full
 *
 */
void BalancedCollector::buildCollectionSet() {
	fprintf(balancedLogFile, "\n\nBuilding collection set\n");

	Region* currentRegion;
	size_t setSize = (size_t)(COLLECTIONSETSIZE*allRegions.size());

	myCollectionSet.clear();
	myCollectionSet.resize(allRegions.size(), 0);
	regionsInSet = 0;

	std::vector<unsigned int> edenRegions = myAllocator->getEdenRegions();
	size_t i, j;

	//Add all Eden Regions first
	for (i = 0; i < allRegions.size(); i++) {
		for (j = 0; j < edenRegions.size(); j++) {
			if (i == edenRegions[j] && !allRegions[i]->getIsLeaf()) {
				regionsInSet++;
				myCollectionSet[i] = 1;
				currentRegion = allRegions[i];
				totalObjectsInCollectionSet += currentRegion->getNumObj();
				//fprintf(balancedLogFile, "Added eden region %zu to collection set\n", i);
			}
		}
	}

#if DEAD_SPACE == 0
	//Add more regions with age > 0
	//linear function passing two points (0,1) (age 0 always selected) and (MAXREGIONAGE, MAXAGEP)
	//there is maximum age which can also be picked with some non-0 probability
	int regionAge;
	float probability;
	int dice;
	for (i = 0; (i < allRegions.size() && regionsInSet<=setSize); i++) {
		if (myCollectionSet[i]==0) {
			currentRegion = allRegions[i];
			if (currentRegion->getNumObj() > 0 && !allRegions[i]->getIsLeaf()) {
				regionAge = currentRegion->getAge();
				probability = (100-MAXAGEP)/(0-MAXREGIONAGE)*regionAge+100;
				dice = rand()%100+1;
				if (dice<=probability) {
					myCollectionSet[i] = 1;
					//fprintf(balancedLogFile, "Added region %i of age %i to collection set\n", i, regionAge);
					//fprintf(balancedLogFile, "Added region %i of age %i and with %zu percent dead space to collection set\n", (deadSpace.at(i)).regionID, regionAge, (deadSpace.at(i)).percentDead);
					regionsInSet++;
					totalObjectsInCollectionSet += currentRegion->getNumObj();
				}
			}
		}
	}
#elif DEAD_SPACE_THRESHOLD == 0
	// Sort percent dead space in descending order
	for (i = 1; i < deadSpace.size(); i++){
		if((deadSpace.at(i)).percentDead > (deadSpace.at(i-1)).percentDead){
			j = 0;
			while((deadSpace.at(i)).percentDead < (deadSpace.at(j)).percentDead){
				j++;
			}
			deadSpace.insert(deadSpace.begin() + j, deadSpace.at(i));
			deadSpace.erase(deadSpace.begin() + i + 1);
		}
	}
	// Add more regions with the highest percent dead space
	for(i = 0; (i < deadSpace.size() && regionsInSet<=setSize); i++) {
		if(myCollectionSet[(deadSpace.at(i)).regionID] == 0 && (deadSpace.at(i)).percentDead > 0 && !allRegions[i]->getIsLeaf()){
			myCollectionSet[(deadSpace.at(i)).regionID] = 1;
			regionsInSet++;
			totalObjectsInCollectionSet += allRegions[(deadSpace.at(i)).regionID]->getNumObj();
			//fprintf(balancedLogFile, "Added region %i with %zu percent dead space to collection set\n", (deadSpace.at(i)).regionID, (deadSpace.at(i)).percentDead);
		}
	}

#else
	// Add more regions with percent dead meeting the minimum threshold
	for(i = 0; (i < allRegions.size() && regionsInSet<=setSize); i++) {
		if(myCollectionSet[i] == 0 && allRegions[i]->getNumObj() > 0 && !allRegions[i]->getIsLeaf()){
			if((deadSpace.at(i)).percentDead >= DEAD_SPACE_THRESHOLD){
				myCollectionSet[i] = 1;
				regionsInSet++;
				totalObjectsInCollectionSet += allRegions[i]->getNumObj();
				//fprintf(balancedLogFile, "Added region %zu with %zu percent dead space to collection set\n", i, (deadSpace.at(i)).percentDead);
			}
		}
	}
#endif
	regionsReclaimed = regionsInSet;
	fprintf(balancedLogFile, "Building collection set done\n");
}

/** Clears the collection set and adds all used regions to the collection set
 *
 */
void BalancedCollector::buildFinalCollectionSet(){
	//fprintf(balancedLogFile, "\n\nBuilding final collection set\n");
	Region* currentRegion;
	unsigned int i;
	regionsInSet = 0;

	myCollectionSet.clear();
	myCollectionSet.resize(allRegions.size(), 0);

	//Add all regions that contain objects or part are a arrayletleaf of a spine object
	for (i = 0; i < allRegions.size(); i++) {
		currentRegion = allRegions[i];
		if(currentRegion->getNumObj() > 0 && !currentRegion->getIsLeaf()){
			myCollectionSet[i] = 1;
			regionsInSet++;
			totalObjectsInCollectionSet += currentRegion->getNumObj();
			//fprintf(balancedLogFile, "Added region %zu with %zu percent dead space to collection set\n", (deadSpace.at(i)).regionID, (deadSpace.at(i)).percentDead);
		}
	}
	regionsReclaimed = regionsInSet;
}

/** Copies live objects to free regions.
 *
 * @return 0 on success, 1 on failure
 */
int BalancedCollector::copy() {
	fprintf(balancedLogFile, "Start copying\n");
	int returnVal;

	getRootObjects();
	returnVal = copyObjectsInQueues();
	if (returnVal == -1) {
		fprintf(balancedLogFile, "Copying root objects was unsuccessful!\n");
		return -1;
	}

	getRemsetObjects();
	returnVal = copyObjectsInQueues();
	if (returnVal == -1) {
		fprintf(balancedLogFile, "Copying remset objects was unsuccessful!\n");
		return -1;
	}
	fprintf(balancedLogFile, "Copying successfully done\n");
	return 0;
}

/** Adds all objects to the object queue which are directly referenced from
 * the roots.
 *
 */
void BalancedCollector::getRootObjects() {
	fprintf(balancedLogFile, "Get Root Objects\n");
	Object* currentObj;
	size_t i;
	size_t j;
	size_t objectRegion;

	// enqueue all roots, copy only objects from collectionSet
	vector<Object*> roots;
	for (i = 0; i < NUM_THREADS; i++) {
		roots = myObjectContainer->getRoots(i);
		//fprintf(balancedLogFile, "Found %zu root objects for Thread %i\n", roots.size(), i);

		for (j = 0; j < roots.size(); j++) {
			currentObj = roots[j];
			if (currentObj && !currentObj->getVisited()) {

				objectRegion = myAllocator->getObjectRegion(currentObj);

				if (myCollectionSet[objectRegion] == 1) { //Object belongs to Collection-set-Region
					myQueue.push(currentObj);
					currentObj->setVisited(true);
				}
			}
		}
	}
	myUpdatePointerQueue = myQueue;
	fprintf(balancedLogFile, "Getting Root Objects done. Added %zu objects to the queue.\n", myQueue.size());
}

/** Add all objects which can be referenced from the rem set entries in their
 * region. Following pointers down the object tree.
 *
 */
void BalancedCollector::getRemsetObjects() {
	fprintf(balancedLogFile, "Get Remset Objects\n");

	Object* currentObj;
	std::set<void*> currentRemset;
	std::set<void*>::iterator remsetIterator;
	size_t j, k;
	size_t children, leaves;
	size_t i;
	size_t parentRegion, childRegion, leafRegion;
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

				parentRegion = myAllocator->getObjectRegion(currentObj);
				if (myCollectionSet[parentRegion] == 0) {
					if(currentObj->allocationType == allocationTypeObject){
						children = currentObj->getPointersMax();
						for (j = 0; j < children; j++) {
							child = currentObj->getReferenceTo(j);
							if (child && !child->getVisited()) {
								childRegion = myAllocator->getObjectRegion(child);

								if (myCollectionSet[childRegion] == 1) {
									child->setVisited(true);
									myQueue.push(child);
									myUpdatePointerQueue.push(child);
								}
							}
						}
					}
					else if(currentObj->allocationType == allocationTypeDiscontiguousIndexable && currentObj->getID() != -1){
						leaves = currentObj->getPointersMax();
						for(k = 0; k < leaves; k++){
							if(currentObj->getReferenceTo(k)){
								children = currentObj->getReferenceTo(k)->getPointersMax();
								for (j = 0; j < children; j++) {
									child = currentObj->getReferenceTo(k)->getReferenceTo(j);
									if (child && !child->getVisited()) {
										childRegion = myAllocator->getObjectRegion(child);

										if (myCollectionSet[childRegion] == 1) {
											child->setVisited(true);
											myQueue.push(child);
											myUpdatePointerQueue.push(child);
										}
									}
								}
								leafRegion = myAllocator->getObjectRegion(currentObj->getReferenceTo(k));
								if(!allRegions[leafRegion]->getIsLeaf()){ //Leaf doesn't occupy entire region
									if (myCollectionSet[leafRegion] == 1) {
										currentObj->getReferenceTo(k)->setVisited(true);
										myQueue.push(currentObj->getReferenceTo(k));
										myUpdatePointerQueue.push(currentObj->getReferenceTo(k));
									}
								}
							}
						}
					}
				}
			}
		}
	}
	fprintf(balancedLogFile, "Getting Remset Objects done. Added %zu objects to the queue.\n", myQueue.size());
	fprintf(balancedLogFile, "%zu pointers to be modified.\n", myUpdatePointerQueue.size());
}

/** Moves all objects in the queue, and their children, to an available region
 *
 * @return 0 on success, -1 on Out of memory error,
 *           -2 on region size smaller than object
 */
int BalancedCollector::copyObjectsInQueues() {
	fprintf(balancedLogFile, "Copy Objects in queue\n");

	size_t i;
	int returnVal;
	size_t childRegion, leafRegion;
	Object* currentObj;
	Object* child;

	//breadth first through the tree using a queue
	while (!myQueue.empty()) {
		currentObj = myQueue.front();
		myQueue.pop();

		size_t children = currentObj->getPointersMax();

		//fprintf(balancedLogFile, "Copying object:\n");
		//printObjectInfo(currentObj);

		returnVal = copyAndForwardObject(currentObj);
		if (returnVal != 0) {
			if (returnVal == -2) {
				fprintf(stderr, "Return -2: object was larger then a region (arraylets are implemented and this should not happen anymore)!\n");
			}
			else if (returnVal == -1) {
				fprintf(stderr, "No more Regions for copying available. Amount of available free regions: %zu\n", myAllocator->getFreeRegions().size());
				myQueue.push(currentObj);

				fprintf(stderr, "Stopping Copying..\n");
				throw 19;
			}
		}

		//fprintf(balancedLogFile, "After Copying object:\n");
		//printObjectInfo(currentObj);
		//fprintf(balancedLogFile, "\n");
		//fprintf(balancedLogFile, "Getting children for object: %i\n", currentObj->getID());

		if(currentObj->allocationType == allocationTypeDiscontiguousIndexable && currentObj->getID() != -1){
			//If object is spine, check leaves for children. If object is leaf, do not check for children
			size_t j;
			size_t leaves = currentObj->getPointersMax();
			for(i = 0; i < leaves; i++) {
				if(currentObj->getReferenceTo(i)){
					children = currentObj->getReferenceTo(i)->getPointersMax();
					for(j = 0; j < children; j++){
						child = currentObj->getReferenceTo(i)->getReferenceTo(j);
						if(child && !child->getVisited()){
							//fprintf(balancedLogFile, "Found child: %i. Address: %zu\n", child->getID(), (size_t)child->getAddress());

							childRegion = myAllocator->getObjectRegion(child);
							if (myCollectionSet[childRegion] == 1) {
								child->setVisited(true);
								myQueue.push(child);
								myUpdatePointerQueue.push(child);
							}
						}
					}
					leafRegion = myAllocator->getObjectRegion(currentObj->getReferenceTo(i));
					if(!allRegions[leafRegion]->getIsLeaf()){ //Leaf doesn't occupy entire region
						if (myCollectionSet[leafRegion] == 1) {
							currentObj->getReferenceTo(i)->setVisited(true);
							myQueue.push(currentObj->getReferenceTo(i));
							myUpdatePointerQueue.push(currentObj->getReferenceTo(i));
						}
					}
				}
			}
		}
		else if(currentObj->allocationType == allocationTypeObject) {
			for (i = 0; i < children; i++) {
				child = currentObj->getReferenceTo(i);
				if (child && !child->getVisited()) {
					//fprintf(balancedLogFile, "Found child: %i. Address: %zu\n", child->getID(), (size_t)child->getAddress());

					childRegion = myAllocator->getObjectRegion(child);
					if (myCollectionSet[childRegion] == 1) {

						child->setVisited(true);
						myQueue.push(child);
						myUpdatePointerQueue.push(child);
					}
				}
			}
		}
	}

	fprintf(balancedLogFile, "Copying enqueued Objects done. Copied %zu objects in total.\n", statCopiedDuringThisGC);
	return 0;
}

/** Copies the object into an available region
 *
 * @param obj Object to be copied
 * @return 0 on success, -1 on Out of memory error,
 *           -2 if Object is too large to allocate to a region
 */
int BalancedCollector::copyAndForwardObject(Object *obj) {
	unsigned int i, currentCopyToRegionID;
	size_t currentFreeSpace, objectSize, objRegionID;
	void *currentFreeAddress, *addressAfter;
	int objRegionAge, returnVal;
	objRegionID = myAllocator->getObjectRegion(obj);
	objRegionAge = allRegions[objRegionID]->getAge();
	if(objRegionAge >= MAXREGIONAGE){ //If object is at maximum age, copy it into a region of the same age
		objRegionAge = MAXREGIONAGE - 1;
	}
	objectSize = obj->getHeapSize();

	//Find a region to copy to
	for (i = 0; i < copyToRegions[objRegionAge].size(); i++) {
		currentCopyToRegionID = copyToRegions[objRegionAge][i];
		currentFreeSpace = allRegions[currentCopyToRegionID]->getCurrFree();
		if (objectSize <= currentFreeSpace) {
			currentFreeAddress = allRegions[currentCopyToRegionID]->getCurrFreeAddr();
			allRegions[currentCopyToRegionID]->setCurrFreeAddr((void*)((size_t)currentFreeAddress+(size_t)objectSize));
			allRegions[currentCopyToRegionID]->setCurrFree(currentFreeSpace-objectSize);
			allRegions[currentCopyToRegionID]->incrementObjectCount();

			void * addressHeap;
			unsigned char *heap = allRegions[currentCopyToRegionID]->getHeapAddress();
			addressHeap = &heap[(size_t)currentFreeAddress];

			myAllocator->setAllocated(heap, (size_t)currentFreeAddress, objectSize);

			memcpy(addressHeap, obj->getAddress(), objectSize);

			obj->updateAddress(addressHeap);

			addressAfter = obj->getAddress();
			obj->setForwardedPointer(addressAfter);
			obj->setForwarded(true);
			statCopiedDuringThisGC++;
			statCopiedObjects++;

			if(obj->allocationType == allocationTypeDiscontiguousIndexable){
				this->addObjectToSpineRemset(obj);
			}

			//fprintf(balancedLogFile, "Copied object %i from region %zu to region %u. Address after: %zu\n", obj->getID(), objRegionID, currentCopyToRegionID, (size_t)addressAfter);

			return 0;
		}
	}

	// Add more regions if no free regions exist
	if(myAllocator->getFreeRegions().size() == 0){
		returnVal = myAllocator->addRegions();
		if(returnVal != 0){
			return returnVal;
		}
		allRegions = myAllocator->getRegions();
		myCollectionSet.resize(allRegions.size(), 0);
	}

	//No copyToRegion found, so add a new one
	if ( myAllocator->getFreeRegions().size() > 0) {
        currentCopyToRegionID = myAllocator->getNextFreeRegionID();
        currentFreeSpace = allRegions[currentCopyToRegionID]->getCurrFree();
        copyToRegions[objRegionAge].push_back(currentCopyToRegionID);
        allRegions[currentCopyToRegionID]->setAge(objRegionAge+1);
        regionsReclaimed--;
		if (objectSize <= currentFreeSpace) {
			//fprintf(balancedLogFile, "\nGetting new copyToRegion with id: %u\n", currentCopyToRegionID);
			currentFreeAddress = allRegions[currentCopyToRegionID]->getCurrFreeAddr();
			allRegions[currentCopyToRegionID]->setCurrFreeAddr((void*)((size_t)currentFreeAddress+(size_t)objectSize));
			allRegions[currentCopyToRegionID]->setCurrFree(currentFreeSpace-objectSize);
			allRegions[currentCopyToRegionID]->incrementObjectCount();

            void * addressHeap;
            unsigned char *heap = allRegions[currentCopyToRegionID]->getHeapAddress();
            addressHeap = &heap[(size_t)currentFreeAddress];

            myAllocator->setAllocated(heap, (size_t)currentFreeAddress, objectSize);

            memcpy(addressHeap, obj->getAddress(), objectSize);

            obj->updateAddress(addressHeap);

            addressAfter = obj->getAddress();
            obj->setForwardedPointer(addressAfter);
            obj->setForwarded(true);
            statCopiedDuringThisGC++;
            statCopiedObjects++;

            if(obj->allocationType == allocationTypeDiscontiguousIndexable){
                this->addObjectToSpineRemset(obj);
            }
            return 0;
        } else if (objectSize > currentFreeSpace) {
            fprintf(stderr, "objectSize = %zu, currentFreeSpace = %zu\n", objectSize, currentFreeSpace);
            fprintf(stderr, "myAllocator->getFreeRegions().size(): %zu\n", myAllocator->getFreeRegions().size());
            throw 19;
        }
    }
    return -1;
}

/** remove dead objects in regions in the collection set
 *
 */
void BalancedCollector::removeObjects(){
	unsigned int i;
	Region* currentRegion;
	RawObject* raw;
	Object* currentObject;
	size_t heapPosition, regionEnd;

	for(i = 0; i < allRegions.size(); i++){
		if(myCollectionSet[i] == 1 && !allRegions[i]->getIsLeaf()){ //Only look for dead objects in regions that have been collected and aren't arraylet leafs
			currentRegion = allRegions[i];
			Optional<size_t>* heapPositionWrapper = myAllocator->getRegionIndex(currentRegion);
			heapPosition = heapPositionWrapper->getValue();
			delete(heapPositionWrapper);
			regionEnd = (i+1) * myAllocator->getRegionSize();

			while(heapPosition < regionEnd){ //Search the region for objects
				raw = (RawObject *)myAllocator->getNextObjectAddress(heapPosition);

				if(raw!=NULL){
					currentObject = raw->associatedObject;
					heapPosition += myAllocator->getSpaceToNextObject(heapPosition);

					if (currentObject) {
						heapPosition += currentObject->getHeapSize();
						if(!currentObject->isForwarded() && currentObject->getID() != -1){
							myMemManager->requestDelete(currentObject, myGeneration == GENERATIONS - 1 ? 1 : 0);
						}
						else{
							currentObject->setForwarded(false);
						}
					}
					else{
						break;
					}
				}
				else{
					break;
				}
			}
		}

	}
}

/** Updates all the remset pointers pointing to objects which were copied.
 *
 */
void BalancedCollector::updateRemsetPointers() {
	fprintf(balancedLogFile, "Update remset pointers\n");

	unsigned int i;
	std::set<void*> currentRemset;
	std::set<void*>::iterator remsetIterator;
	void* forwardPointer;
	void* remsetPointer;
	size_t objectRegion;

	for (i = 0; i < allRegions.size(); i++) {
		if (myCollectionSet[i] == 0) { //Not from collection set
			currentRemset = allRegions[i]->getRemset();

			//fprintf(balancedLogFile, "Region = %u. Remset size = %zu\n", i, currentRemset.size());
			for (remsetIterator = currentRemset.begin(); remsetIterator != currentRemset.end(); ++remsetIterator) {
				remsetPointer = *remsetIterator;
				//fprintf(balancedLogFile, "remsetPointer = %zu\n", (size_t)remsetPointer);

				RawObject *rawObject = (RawObject *) remsetPointer;
				Object *obj = rawObject->associatedObject;

				if (obj) {
					forwardPointer = obj->getForwardedPointer();

					//fprintf(balancedLogFile, "forwardPointer for object %i = %zu\n", obj->getID(), (size_t)forwardPointer);

					objectRegion = myAllocator->getObjectRegionByRawObject(forwardPointer);
					//fprintf(balancedLogFile, "objectRegion = %u\n", objectRegion);

					if (myCollectionSet[objectRegion] == 1) { //pointing to region from collection set? -> Delete pointer
						allRegions[i]->eraseObjectReferenceWithoutCheck(remsetPointer);
					}
					else {					//Not pointing to region from collecton set. Just erase old entry and insert new entry
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

/** Updates all objects who's children have moved.
 * The update requires getting the new address of each moved child
 * and changing the old pointer value to the new location. Remsets are
 * modified as required.
 */
void BalancedCollector::updatePointers() {
	fprintf(balancedLogFile, "Update pointers\n");

	size_t j, k;
	Object* currentObj;
	Object* child;
	size_t children, leaves;
	size_t parentRegion, childRegion;

	while (!myUpdatePointerQueue.empty()) {
		currentObj = myUpdatePointerQueue.front();
		myUpdatePointerQueue.pop();
		//printObjectInfo(currentObj);
		if(currentObj->allocationType == allocationTypeDiscontiguousIndexable && currentObj->getID() != -1){
			//continue;
			leaves = currentObj->getPointersMax();
			for(j = 0; j < leaves; j++){
				if(currentObj->getReferenceTo(j)){
					children = currentObj->getReferenceTo(j)->getPointersMax();
					for(k = 0; k < children; k++){
						child = currentObj->getReferenceTo(j)->getReferenceTo(k);
						if (child) {
							currentObj->getReferenceTo(j)->setRawPointerAddress(k, child->getForwardedPointer());
							childRegion = myAllocator->getObjectRegion(child);
							parentRegion = myAllocator->getObjectRegion(currentObj);
							if (childRegion!=parentRegion && myCollectionSet[parentRegion] == 0) {
								allRegions[childRegion]->insertObjectReference(currentObj->getAddress());
							}
						}
					}
				}
			}
		}
		else if(currentObj->allocationType == allocationTypeObject){
			children = currentObj->getPointersMax();
			for (j = 0; j < children; j++) {
				child = currentObj->getReferenceTo(j);
				if (child) {
					currentObj->setRawPointerAddress(j, child->getForwardedPointer());
					childRegion = myAllocator->getObjectRegion(child);
					parentRegion = myAllocator->getObjectRegion(currentObj);
					if (childRegion!=parentRegion && myCollectionSet[parentRegion] == 0) {
						allRegions[childRegion]->insertObjectReference(currentObj->getAddress());
					}
				}
			}
		}
		currentObj->setVisited(false);
	}

	fprintf(balancedLogFile, "Updating pointers done\n");
}

/** Regions change type as a part of collection. This method
 * reassigns the types for regions who's types changed.
 *
 */
void BalancedCollector::reOrganizeRegions(){
	unsigned int i;
	Region* currentRegion;

	//Clean up regions, free regions, and eden regions
	for (i = 0; i < myCollectionSet.size(); i++) {

		if (myCollectionSet[i] == 1 && !allRegions[i]->getIsLeaf()) {
			currentRegion = allRegions[i];

			//fprintf(stderr, "Resetting region %u\n", i);
			currentRegion->reset();
			myAllocator->setRegionFree(currentRegion);
			myAllocator->addNewFreeRegion(i);
			myAllocator->removeEdenRegion(i);
		}
		else if(allRegions[i]->getIsLeaf()){
			myAllocator->removeEdenRegion(i);
		}
	}

 }

/** Prints information about where an object is and where it's children are.
 *
 * @param obj Object who's statistical information needs to be printed
 */
void BalancedCollector::printObjectInfo(Object* obj){
	size_t k, l;
	Object* child;
	void* rawChildAddress;
	fprintf(balancedLogFile, "ID = %i. Address = %zu. ", obj->getID(), (size_t)obj->getAddress());
	if(obj->allocationType != allocationTypeDiscontiguousIndexable){
		size_t children = obj->getPointersMax();
		for (k = 0; k < children; k++) {
			child = obj->getReferenceTo(k);
			rawChildAddress = obj->getRawPointerAddress(k);
			if (child) {
				fprintf(balancedLogFile, "Child %zu: ID = %i. Address = %zu. ", k, child->getID(), (size_t)rawChildAddress);
			}
		}
	}
	else if(obj->allocationType == allocationTypeDiscontiguousIndexable && obj->getID() != -1){
		size_t leaves = obj->getPointersMax();
		for (k = 0; k < leaves; k++){
			if(obj->getReferenceTo(k)){
				size_t children = obj->getReferenceTo(k)->getPointersMax();
				for(l = 0; l < children; l++){
					child = obj->getReferenceTo(k)->getReferenceTo(l);
					rawChildAddress = obj->getReferenceTo(k)->getRawPointerAddress(l);
					if(child){
						fprintf(balancedLogFile, "Child %zu: ID = %i. Raw Address = %zu. Address = %zu. ", k*obj->getReferenceTo(0)->getPointersMax()+l, child->getID(), (size_t)rawChildAddress, (size_t)child->getAddress());
					}
				}
			}
		}
	}
	fprintf(balancedLogFile, "\n");
}

/** Prints statistical information about the collections on the heap
 *
 */
void BalancedCollector::printFinalStats() {
	fprintf(balancedLogFile, "Garbage Collection #%zu done!\n", statGcNumber);
	fprintf(balancedLogFile, "Amount of free regions: %zu. Amount of eden regions: %zu \nRegions collected: %zu, regions reclaimed: %zu\n\n****************************************************************\n\n", myAllocator->getFreeRegions().size(), myAllocator->getEdenRegions().size(), regionsInSet, regionsReclaimed);
}

BalancedCollector::~BalancedCollector() {
	//deadSpace.clear()
}

}
