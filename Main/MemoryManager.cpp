/*
 * MemoryManager.cpp
 *
 *  Created on: 2013-09-03
 *      Author: GarCoSim
 */


#include "Object.hpp"
#include "ObjectContainer.hpp"
#include "../Allocators/Allocator.hpp"
#include "../Allocators/RealAllocator.hpp"
#include "../Allocators/BasicAllocator.hpp"
#include "../Allocators/NextFitAllocator.hpp"
#include "../Collectors/Collector.hpp"
#include "../Collectors/MarkSweepCollector.hpp"
#include "../Collectors/TraversalCollector.hpp"
#include "../Collectors/RecyclerCollector.hpp"
#include "../WriteBarriers/WriteBarrier.hpp"
#include "../WriteBarriers/RecyclerWriteBarrier.hpp"
#include "../WriteBarriers/ReferenceCountingWriteBarrier.hpp"

#include <sstream>
#include <stdlib.h>
#include <math.h>
#include <fstream>
#include "MemoryManager.hpp"

extern FILE* gLogFile;
extern FILE* gDetLog;
extern FILE* balancedLogFile;
extern int gLineInTrace;
extern string globalFilename;

namespace traceFileSimulator {

MemoryManager::MemoryManager(size_t heapSize, int highWatermark, int collector, int traversal, int allocator, int writebarrier) {
	_allocator = (allocatorEnum)allocator;
	_collector = (collectorEnum)collector;
	_traversal = (traversalEnum)traversal;
	_writebarrier = (writebarriersEnum)writebarrier;

	classTableLoaded = false;

	initWritebarrier();
	initAllocators(heapSize);
	initContainers();
	initGarbageCollectors(highWatermark);

	if (myWriteBarrier)
		myWriteBarrier->setEnvironment(myGarbageCollectors[0]);
}

bool MemoryManager::loadClassTable(string traceFilePath) {
	ifstream classFile;
	size_t found;
	string className = globalFilename + ".cls";
	string line;

	// we need to push an empty element into the vector as our classes start with id 1
	classTable.push_back("EMPTY");

	classFile.open(className.c_str());
	if (!classFile.good())
		return false;

	do {
		if(getline(classFile, line)) {
			found = line.find(": ");
			line = line.substr(found + 2, line.size() - found - 2);
			classTable.push_back(line);
		}
	} while (!classFile.eof());

	classTableLoaded = true;

	return true;;
}

char *MemoryManager::getClassName(int classNumber) {
	if (!hasClassTable())
		return (char*)"CLASS_TABLE_NOT_LOADED";

	if (classNumber > (int)classTable.size())
		return (char*)"OUT_OF_BOUNDS";

	return (char*)classTable.at(classNumber).c_str();
}

bool MemoryManager::hasClassTable() {
	return classTableLoaded;
}

void MemoryManager::initAllocators(size_t heapsize) {
	int i;
	size_t* genSizes = computeHeapsizes(heapsize);
	for (i = 0; i < GENERATIONS; i++) {
		switch (_allocator) {
			case realAlloc:
				myAllocators[i] = new RealAllocator();
				break;
			case basicAlloc:
				myAllocators[i] = new BasicAllocator();
				break;
			case nextFitAlloc:
				myAllocators[i] = new NextFitAllocator();
				break;
		}
		myAllocators[i]->initializeHeap(genSizes[i]);
	}
	free(genSizes);
}

void MemoryManager::initContainers() {
	int i;
	for (i = 0; i < GENERATIONS; i++) {
		myObjectContainers[i] = new ObjectContainer();
	}
}

void MemoryManager::initGarbageCollectors(int highWatermark) {
	int i;
	for (i = 0; i < GENERATIONS; i++) {
		switch (_collector) {
			case markSweepGC:
				myGarbageCollectors[i] = new MarkSweepCollector();
				break;
			case traversalGC:
				myGarbageCollectors[i] = new TraversalCollector();
				break;
			case recyclerGC:
				myGarbageCollectors[i] = new RecyclerCollector();
				break;
		}
		myGarbageCollectors[i]->setEnvironment(myAllocators[i],	myObjectContainers[i], (MemoryManager*) this, highWatermark, i, _traversal);
		myGarbageCollectors[i]->initializeHeap();
	}
}

void MemoryManager::initWritebarrier() {
	switch (_writebarrier) {
		case disabled:
			myWriteBarrier = NULL;
			break;
		case recycler:
			myWriteBarrier = new RecyclerWriteBarrier();
			break;
		case referenceCounting:
			myWriteBarrier = new ReferenceCountingWriteBarrier();
			break;
	}
}

void MemoryManager::statBeforeCompact(int myGeneration) {
	int i;
	for(i = 0;i<=myGeneration;i++){
		stats[i] = myAllocators[i]->getFreeSize();
	}
}

void MemoryManager::statAfterCompact(int myGeneration) {
	int i;
	for(i = 0;i<=myGeneration;i++){
		stats[i] -= myAllocators[i]->getFreeSize();
		if(stats[i] != 0){
			fprintf(stderr,"ERROR(Line %d):compact incosistency %d bytes for gen %d\n",
					gLineInTrace, stats[i],i);
		}
	}

}

void *MemoryManager::shift(size_t size){
	//the idea: if there is still space for this object in the highest generation,
	//gc until promotes happen rather than crash the application
	void *result = NULL;
	int outOfMemory = 0;
	size_t spaceOnTop = myAllocators[GENERATIONS-1]->getFreeSize();
	while(result == NULL && spaceOnTop >= size){
		if(WRITE_DETAILED_LOG==1){
			fprintf(gDetLog,"(%d) SHIFTING for %zu\n",gLineInTrace,size);
		}
		myGarbageCollectors[GENERATIONS-1]->collect((int)reasonShift);
		outOfMemory = myGarbageCollectors[GENERATIONS-1]->promotionPhase();
		if(outOfMemory==-1){
			fprintf(stderr,"(%d) OUT OF MEMORY: (%zu)\n",gLineInTrace,size);
			exit(1);
		}
		result = myAllocators[0]->gcAllocate(size);
	}
	return result;
}

int MemoryManager::evalCollect(){
	myGarbageCollectors[GENERATIONS-1]->collect((int)reasonEval);
	return 0;
}


void *MemoryManager::allocate(size_t size, int generation) {
	//check if legal generation
	if (generation < 0 || generation > GENERATIONS - 1) {
		fprintf(stderr, "ERROR (Line %d): allocate to illegal generation: %d\n",
				gLineInTrace, generation);
		exit(1);
	}
	void *result = NULL;
	int gen = generation;
	//try allocating in the generation
	result = myAllocators[generation]->gcAllocate(size);
	while (result == NULL && gen < GENERATIONS) {
		if (WRITE_DETAILED_LOG == 1) {
			fprintf(gDetLog,
					"(%d) Trigger Gc in generation %d.\n",
					gLineInTrace, gen);
		}

		myGarbageCollectors[gen]->collect(reasonFailedAlloc);

		result = myAllocators[generation]->gcAllocate(size);

		gen++;
	}
	if (gen > generation) {
		//gcs were made. promote if possible
		myGarbageCollectors[gen - 1]->promotionPhase();
	}

	if(GENERATIONS > 1 && result == NULL && SHIFTING == 1){
		//try shifting
		result = shift(size);
	}

	return result;
}


void *MemoryManager::allocate(size_t size, int generation, int thread) { //if region-based; by Tristan
	//check if legal generation
	if (generation < 0 || generation > GENERATIONS - 1) {
		fprintf(stderr, "ERROR (Line %d): allocate to illegal generation: %d\n",
				gLineInTrace, generation);
		exit(1);
	}
	void *result = NULL;
	int gen = generation;
	//try allocating in the generation
	result = myAllocators[generation]->gcAllocate(size,thread);
	while ((long)result < 0 && gen < GENERATIONS) {
		if (WRITE_DETAILED_LOG == 1) {
			fprintf(gDetLog,
					"(%d) Trigger Gc in generation %d.\n",
					gLineInTrace, gen);
		}

		result = myAllocators[generation]->gcAllocate(size);

		gen++;
	}
	if (gen > generation) {
		//gcs were made. promote if possible
		myGarbageCollectors[gen - 1]->promotionPhase();
	}

	if(GENERATIONS > 1 && result == NULL && SHIFTING == 1){
		//try shifting
		result = shift(size);
	}

	return result;
}


void MemoryManager::addRootToContainers(Object* object, int thread) {

	int i;
	for (i = 0; i < GENERATIONS; i++) {
		if (i == GENERATIONS - 1) {
			myObjectContainers[i]->addToRoot(object, thread);
			//fprintf(stderr,"(%d)DEBUG: rootset %d\n",gLineInTrace, myObjectContainers[i]->getRootSize());
			//if(myObjectContainers[1]->getRootSize() != myObjectContainers[0]->getGenRootCount()){
				//exit(1);
				//fprintf(stderr,"(%d)DEBUG: EXIT\n",gLineInTrace);
			//}
		} //otherwise if there is more than one generation, add new object to remSets
		else {
			myObjectContainers[i]->add(object);
			myObjectContainers[i]->addToGenRoot(object);
			if (WRITE_DETAILED_LOG == 1) {
				fprintf(gDetLog, "(%d) Adding %d to remset %d\n", gLineInTrace,
						object->getID(), i);
			}
			//fprintf(stderr,"(%d)DEBUG: remset %d\n",gLineInTrace, myObjectContainers[i]->getGenRootCount());

		}
	}
}

int MemoryManager::allocateObjectToRootset(int thread, int id,size_t size, int refCount, int classID) {
	if (WRITE_DETAILED_LOG == 1)
		fprintf(gDetLog, "(%d) Add Root to thread %d with id %d\n", gLineInTrace, thread, id);

	void* address;

	if (size > 32000) { //bigger than region with heap size of 32m
		address = allocate(32000, 0);
		return postAllocateObjectToRootset(thread,id,32000,refCount,classID,address);
	}
	else {
		address = allocate(size, 0);
		return postAllocateObjectToRootset(thread,id,size,refCount,classID,address);
	}

	address = allocate(size, 0);

    return postAllocateObjectToRootset(thread,id,size,refCount,classID,address);
}

int MemoryManager::regionAllocateObjectToRootset(int thread, int id,size_t size, int refCount, int classID) {//if region-based; by Tristan

	if (WRITE_DETAILED_LOG == 1)
		fprintf(gDetLog, "(%d) Add Root to thread %d with id %d\n", gLineInTrace, thread, id);

    void* address = allocate(size, 0,thread);

    return postAllocateObjectToRootset(thread,id,size,refCount,classID,address);
}

inline int MemoryManager::postAllocateObjectToRootset(int thread, int id,size_t size, int refCount, int classID,void *address) {//post allocation; by Tristan
	if (address == NULL) {
		fprintf(gLogFile, "Failed to allocate %zu bytes in trace line %d.\n",size, gLineInTrace);
		fprintf(stderr, "ERROR(Line %d): Out of memory (%zu bytes)\n",gLineInTrace,size);
		myGarbageCollectors[GENERATIONS-1]->lastStats();
		exit(1);
	}
	//fprintf(balancedLogFile, "ID: %i\n", id);
	//create Object
	Object *object;
	// modified by mazder, added thread id
	if(escapeAnalysis){
 	    object = new Object(thread, id, address, size, refCount, getClassName(classID));
		totalObject++;
	}
	else {
		object = new Object(id, address, size, refCount, getClassName(classID));
		totalObject++;
	}


	// increase class usage
	if(clsInfo && ( (int)classStat.size() !=0 ) ){
		classStat[classID] = classStat[classID]+1;
	}

	object->setGeneration(0);
	//add to Containers
	addRootToContainers(object, thread);

	if (DEBUG_MODE == 1) {
		myGarbageCollectors[GENERATIONS - 1]->collect(reasonDebug);
		myGarbageCollectors[GENERATIONS - 1]->promotionPhase();
	}

	return 0;
}

int MemoryManager::requestRootDelete(int thread, int id){
	Object* oldRoot = myObjectContainers[GENERATIONS - 1]->getRoot(thread, id);
	myObjectContainers[GENERATIONS - 1]->removeFromRoot(thread, id);
	//remove the root from rem sets.
	int i;
	for(i=0;i<GENERATIONS-1;i++){
		myObjectContainers[i]->removeFromGenRoot(oldRoot);
	}

	if (myWriteBarrier) {
		myWriteBarrier->process(oldRoot, NULL);
		if (ZOMBIE)
			myGarbageCollectors[0]->collect(reasonFailedAlloc);
	}

	return 0;
}

bool MemoryManager::isAlreadyRoot(int thread, int id) {
	return myObjectContainers[GENERATIONS-1]->isAlreadyRoot(thread, id);
}

int MemoryManager::requestRootAdd(int thread, int id){
	if (isAlreadyRoot(thread, id))
		return -1;

	Object* obj = myObjectContainers[GENERATIONS-1]->getByID(id);
	if (obj)
		myObjectContainers[GENERATIONS-1]->addToRoot(obj, thread);
	else
		printf("Unable to add Object %i to roots\n", id);

	if (myWriteBarrier)
		myWriteBarrier->process(NULL, obj);

	return 0;
}

void MemoryManager::requestDelete(Object* object, int gGC) {
	if (WRITE_DETAILED_LOG == 1) {
		fprintf(gDetLog, "(%d) Delete object with id %d\n", gLineInTrace,
				object->getID());
	}
	int i;
	int objGeneration = object->getGeneration();
	//int objID = object->getID();
	//delete object from all Gen Roots it might be in
	for (i = objGeneration + 1; i < GENERATIONS; i++) {
//		if (i != GENERATIONS - 1) {
//			while (myObjectContainers[i]->removeFromGenRoot(object) != -1) {
//				if (WRITE_DETAILED_LOG == 1) {
//					fprintf(gDetLog,
//							"(%d) Removing %d from remset %d (deleteObj)\n",
//							gLineInTrace, object->getID(), i);
//				}
//			}
//		}
		int status = myObjectContainers[i]->removeReferenceTo(object);
		if (status == -1) {
			fprintf(stderr,
					"ERROR(Line %d):Object %d(g%d) could not be removed from object container %d\n",
					gLineInTrace, object->getID(), objGeneration, i);
		}
	}

	//now free in allocator and delete object
	myAllocators[objGeneration]->gcFree(object);
	myObjectContainers[objGeneration]->deleteObject(object, !myAllocators[objGeneration]->isRealAllocator());
}

void MemoryManager::requestFree(Object* object) {

//	if (WRITE_DETAILED_LOG == 1) {
//		fprintf(gDetLog, "(%d) Free request id %d\n", gLineInTrace,
//				object->getID());
//	}

	//if (object && object->getFreed() != 1) {
	if (object) {
		int gen = object->getGeneration();
		myAllocators[gen]->gcFree(object);
	}
}

void MemoryManager::requestReallocate(Object* object) {
//	if (WRITE_DETAILED_LOG == 1) {
//		fprintf(gDetLog, "(%d) Reallocate request for id %d\n", gLineInTrace,
//				object->getID());
//	}

	if (object) {
		int gen = object->getGeneration();
		size_t size = object->getHeapSize();
		void *address = myAllocators[gen]->gcAllocate(size);
		memcpy(address, object->getAddress(), size);
		if (address == NULL) {
			fprintf(stderr,"ERROR(Line %d):Could not reallocate Object %d to gen %d\n",gLineInTrace, object->getID(), gen);
			exit(1);
		}
		object->updateAddress(address);
		//TODO what about the old RawObject? How does it get freed?
		// In markSweep collection, the entire heap is explicitly freed (but no
		// objects are deleted) during the compaction phase.

		//object->setFreed(0);

	}
}

void MemoryManager::requestResetAllocationPointer(int generation) {
	if (WRITE_DETAILED_LOG == 1) {
		fprintf(gDetLog, "(%d) Request to reset allocation pointers\n",
				gLineInTrace);
	}
	int i;
	for (i = 0; i <= generation; i++) {
		myAllocators[i]->resetRememberedAllocationSearchPoint();
	}
}

int MemoryManager::requestPromotion(Object* object) {
	if (object->getGeneration() == GENERATIONS - 1) {
		if (WRITE_DETAILED_LOG == 1) {
			fprintf(gDetLog,"(%d) Request to promote %d, but as it is in maxGen, not granted.\n",gLineInTrace, object->getID());
		}
		return 0;
	}

	int oldGen = object->getGeneration();
	int newGen = oldGen + 1;
	size_t size = object->getHeapSize();

	if (WRITE_DETAILED_LOG == 1) {
		fprintf(gDetLog, "(%d) Request to promote %d from %d to %d\n",gLineInTrace, object->getID(), oldGen, newGen);
	}

	void *address = myAllocators[newGen]->gcAllocate(size);
	memcpy(address, object->getAddress(), size);
	if (address == NULL) {
		//there is not enough space upstairs, stay where you are for a little longer
		if (WRITE_DETAILED_LOG == 1) {
			fprintf(gDetLog,"(%d) Request to promote %d from %d to %d not possible (no space)\n",gLineInTrace, object->getID(), oldGen, newGen);
		}
		//this line signalizes that there was an out of space error
		return 1;
	}

	//promote object
	myAllocators[oldGen]->gcFree(object);
	object->updateAddress(address);
	//TODO what about the old RawObject? How does it get freed?
	object->setGeneration(newGen);
	//remove from old generation
	myObjectContainers[oldGen]->removeReferenceTo(object);
	//remove all remSet entries
	while (myObjectContainers[oldGen]->removeFromGenRoot(object) != -1) {
		if (WRITE_DETAILED_LOG == 1) {
			fprintf(gDetLog,"(%d) Removing myself %d from remset %d (promotion))\n",gLineInTrace, object->getID(), oldGen);
		}
	}
	//handle children
	int i;
	for (i = 0; i < object->getPointersMax(); i++) {
		Object* child = object->getReferenceTo(i);
		if (child && child->getGeneration() == oldGen) {
			myObjectContainers[oldGen]->addToGenRoot(child);
			if (WRITE_DETAILED_LOG == 1) {
				fprintf(gDetLog,"(%d) Adding %d to remset %d (parent (%d) was promoted))\n",gLineInTrace, child->getID(), oldGen, object->getID());
			}
		}
	}
	return 0;
}

void MemoryManager::addToContainers(Object* object) {
	int i;
	for (i = 0; i < GENERATIONS; i++) {
		myObjectContainers[i]->add(object);
	}
}


int MemoryManager::setPointer(int thread, int parentID, int parentSlot, int childID) {
	int parentGeneration = -1;
	Object *parent,*child,*oldChild;

	if (WRITE_DETAILED_LOG == 1) {
		fprintf(gDetLog, "(%d) Set pointer from %d(%d) to %d\n", gLineInTrace,parentID, parentSlot, childID);
	}


	parent = myObjectContainers[GENERATIONS - 1]->getByID(parentID);
	//id 0 represents the NULL object.
	child = NULL;
	int childGeneration = -1;
	if(childID != 0) {
		child = myObjectContainers[GENERATIONS - 1]->getByID(childID);
		if (child){
			childGeneration = child->getGeneration();
		}
		else{
			std::stringstream ss;
			ss << "Child object " << childID << " does not exist. Ignoring trace file statement and continuing.\n";
			ERRMSG(ss.str().c_str());
			return 0;
		}
	}

	if (parent){
		parentGeneration = parent->getGeneration();
		oldChild = parent->getReferenceTo(parentSlot);
		parent->setPointer(parentSlot, child);
	}
	else{
		std::stringstream ss;
		ss << "Parent object " << parentID << " does not exist. Ignoring trace file statement and continuing.\n";
		ERRMSG(ss.str().c_str());
		return 0;
	}

	//check old child, if it has remSet entries then delete them
	if (oldChild && parentGeneration > oldChild->getGeneration()) {
		int i;
		for (i = oldChild->getGeneration(); i < parentGeneration; i++) {
			if (WRITE_DETAILED_LOG == 1) {
				fprintf(gDetLog,"(%d) removing %d from remset %d (i am oldchild of (%d) in setpointer)\n",gLineInTrace, child->getID(), i, parent->getID());
			}
			int status = myObjectContainers[i]->removeFromGenRoot(oldChild);
			if (status == -1) {
				fprintf(stderr,"ERROR (Line %d): could not remove oldChild %d from remset %d\n",gLineInTrace, oldChild->getID(), i);
				//exit(1);
			}
		}
	}
	//Add child to remsets
	if (childID && parentGeneration > childGeneration) {
		int i;
		for (i = childGeneration; i < parentGeneration; i++) {
			if (child)
				myObjectContainers[i]->addToGenRoot(child);
			if (WRITE_DETAILED_LOG == 1) {
				fprintf(gDetLog,"(%d) Adding %d to remset %d (parent (%d) got a new pointer to me))\n",gLineInTrace, child->getID(),i, parent->getID());
			}
		}
	}


	if (DEBUG_MODE == 1) {
		myGarbageCollectors[GENERATIONS - 1]->collect(reasonDebug);
		myGarbageCollectors[GENERATIONS - 1]->promotionPhase();
	}

	if (myWriteBarrier) {
		myWriteBarrier->process(oldChild, child);
		if (ZOMBIE)
			myGarbageCollectors[0]->collect(reasonFailedAlloc);
	}

	// added by mazder for escape analysis
	/****************************************/
	if(escapeAnalysis){
		// parent ----> child
		if (childID !=0 ){
			if( (parent->myTid) !=  (child->myTid) ){
				// child and  its all successors are to be escaped
				if(!child->escaped) {
					// mark child and its all successors are escaped
					markObject(child);
				}
			}
			else{
				if( (parent->escaped) && (!child->escaped) ){
					// mark child and its all successors are escaped
					markObject(child);
				}
			}
		}
	}
	/****************************************/
	return 0;
}

void MemoryManager::setStaticPointer(int classID, int fieldOffset, int objectID) {
	Object* myChild;
	Object* myOldChild;

	myOldChild = myObjectContainers[GENERATIONS - 1]->getStaticReference(classID, fieldOffset);

	myObjectContainers[GENERATIONS - 1]->setStaticReference(classID, fieldOffset, objectID);

	if (myWriteBarrier) {
		myChild = myObjectContainers[GENERATIONS - 1]->getStaticReference(classID, fieldOffset);
		myWriteBarrier->process(myOldChild, myChild);
		if (ZOMBIE)
			myGarbageCollectors[0]->collect(reasonFailedAlloc);
	}
}

int MemoryManager::regionSetPointer(int thread, int parentID, int parentSlot,int childID) {
	unsigned long parentRegion,childRegion;

	preSetPointer(thread,parentID,parentSlot,childID);

	parentRegion = myAllocators[0]->getObjectRegion(parent);

	if (child) {
		childRegion =  myAllocators[0]->getObjectRegion(child);
		if (parentRegion != childRegion) {
			myAllocators[0]->getRegions()[childRegion]->insertObjectReference(parent->getAddress());
		}

	}

	return 0;
}

void MemoryManager::clearRemSets(){
	int i;
	for(i = 0;i<GENERATIONS;i++){
		myObjectContainers[i]->clearRemSet();
	}
}

void MemoryManager::requestRemSetAdd(Object* currentObj){
	int i;
	int objGen = currentObj->getGeneration();
	for(i= objGen ; i <  GENERATIONS-1; i++){
		myObjectContainers[i]->addToGenRoot(currentObj);
	}
}

void MemoryManager::forceGC() {
	myGarbageCollectors[GENERATIONS-1]->collect((int)reasonForced);
}

void MemoryManager::lastStats() {
	myGarbageCollectors[GENERATIONS-1]->lastStats();
}


size_t* MemoryManager::computeHeapsizes(size_t heapSize) {
	size_t heapLeft = heapSize;
	size_t* result = (size_t*) malloc(GENERATIONS * sizeof(size_t));
	int i;

	for (i = GENERATIONS - 1; i >= 0; i--) {
		if (i == 0) { // the youngest space gets what is left over
			result[i] = heapLeft;
		} else {
			result[i] = ceil(heapLeft * (1.0 - GENRATIO)); //no byte is left behind
			heapLeft = heapLeft * GENRATIO;
		}
		if (GEN_DEBUG == 1) {
			printf("GENDEBUG: G%d: %zu\n", i, result[i]);
		}
	}

	if (GEN_DEBUG == 1) {
		size_t sum = 0;
		for (i = 0; i < GENERATIONS; i++) {
			sum = sum + result[i];
		}
		printf("GENDEBUG: Sum of Generations: %zu\n", sum);
	}

	return result;
}

void MemoryManager::printStats() {
	for(int i=0; i < GENERATIONS;i++){
		myGarbageCollectors[i]->printStats();
	}

}

void MemoryManager::dumpHeap() {
	myObjectContainers[GENERATIONS-1]->dumpHeap();
}


MemoryManager::~MemoryManager() {
	for (int i = 0; i < GENERATIONS; i++) {
		delete(myGarbageCollectors[i]);
		delete(myObjectContainers[i]);
		delete(myAllocators[i]);
	}
	classTable.clear();
	//delete(classTable);
}

}
