/*
 * MemoryManager.cpp
 *
 *  Created on: 2013-09-03
 *      Author: GarCoSim
 */

#include "../defines.hpp"
#include "Object.hpp"
#include "ObjectContainer.hpp"
#include "../Allocators/Allocator.hpp"
#include "../Allocators/BasicAllocator.hpp"
#include "../Allocators/NextFitAllocator.hpp"
#include "../Allocators/RegionBasedAllocator.hpp"
#include "../Collectors/Collector.hpp"
#include "../Collectors/MarkSweepCollector.hpp"
#include "../Collectors/TraversalCollector.hpp"
#include "../Collectors/RecyclerCollector.hpp"
#include "../Collectors/BalancedCollector.hpp"
#include "../WriteBarriers/WriteBarrier.hpp"
#include "../WriteBarriers/RecyclerWriteBarrier.hpp"
#include "../WriteBarriers/ReferenceCountingWriteBarrier.hpp"

#include <sstream>
#include <stdlib.h>
#include <math.h>
#include <fstream>
#include "MemoryManager.hpp"

#include <iostream>
#include <string> // std::stoi
#include <sstream>

extern FILE* gLogFile;
extern FILE* gDetLog;
extern FILE* balancedLogFile;
extern TRACE_FILE_LINE_SIZE gLineInTrace;
extern string globalFilename;

namespace traceFileSimulator {

int (MemoryManager::*preAllocateObject)(int,int,size_t,size_t,int) = NULL;
int chunkID = 2147483647; //used as objects ID for new objects created by breaking up large objects into several smaller objects

/** Creates a Memory Manager object. Responsible for correctly instantiating and using the defined collector,
 * allocator, write barrier and traversal algorithm.
 *
 * @param heapSize Initial size of the heap
 * @param maxHeapSize Maximum size of the heap
 * @param highWatermark Used to garbage collect after a certain ratio of space is used
 * @param collector Collector to be used
 * @param traversal Traversal Order to be used
 * @param allocator Allocator to be used
 * @param writebarrier Write barrier to be used
 */
MemoryManager::MemoryManager(size_t heapSize, size_t maxHeapSize, int highWatermark, int collector, int traversal, int allocator, int writebarrier) {
	_allocator = (allocatorEnum)allocator;
	_collector = (collectorEnum)collector;
	_traversal = (traversalEnum)traversal;
	_writebarrier = (writebarriersEnum)writebarrier;

	classTableLoaded = false;

	initWritebarrier();
	initAllocators(heapSize, maxHeapSize);
	initContainers();
	initGarbageCollectors(highWatermark);

	if (myWriteBarrier)
		myWriteBarrier->setEnvironment(myGarbageCollectors[0]);

	if (_allocator == threadBased)
		preAllocateObject = &MemoryManager::preAllocateObjectThreadBased;
	else
		preAllocateObject = &MemoryManager::preAllocateObjectDefault;
}

/** Loads in the class names from a file.
*
* @param traceFilePath
* @return true if successful, false if not successful or if the class table is already loaded
*/
bool MemoryManager::loadClassTable(string traceFilePath) {
	ifstream classFile;
	size_t found;
	string className = globalFilename + ".cls";
	string line;
	char delim = ' ';
	string token;
	vector<string> splitLine;
	int id;

	classFile.open(className.c_str());
	if (!classFile.good())
		return false;

	while (!classFile.eof()){
		if(getline(classFile, line)) {
			stringstream ss(line);
			splitLine.clear();
			while(getline(ss, token, delim)){
				splitLine.push_back(token);
			}
			string front = splitLine.front();
			found = front.find("C");
			if(found!=std::string::npos){
				istringstream convert(front.substr(found+1,front.length()));
				convert >> id;
			}
			else{
				std::cout<<"loading the class table a line does not start with 'C'"<<std::endl<<line<<std::endl;
				classFile.close();
				return false;
			}
			classTable.insert( std::pair<int,string>(id, splitLine.back()) );
		}
	}
	classTableLoaded = true;

	classFile.close();

	return true;
}

/** Searches the class table for the class id and get's it's name from the table.
 *  If the class id is outside the tables bounds it will return a string of "OUT_OF_BOUNDS".
 *  If the class table has not been loaded then it will return a string of "CLASS_TABLE_NOT_LOADED".
 * @param classId Id of the class to be looked up
 * @return Name of the class from the class table, or one of the two possible error strings
 */
char *MemoryManager::getClassName(int classId) {
	if (!hasClassTable())
		return (char*)"CLASS_TABLE_NOT_LOADED";

	if (classId > (int)classTable.size())
		return (char*)"OUT_OF_BOUNDS";

	return (char*)classTable.at(classId).c_str();
}

bool MemoryManager::isArray(int classId) {
	return getClassName(classId)[0] == '[';
}

bool MemoryManager::hasClassTable() {
	return classTableLoaded;
}

/** Sets the allocator to the correct concrete class by calling it's
 * constructor and then it's initializeHeap member function with the
 * supplied arguments.
 *
 * @param heapsize Initial size of the heap
 * @param maxheapsize Maximum size of the heap
 */
void MemoryManager::initAllocators(size_t heapsize, size_t maxheapsize) {
	int i;
	size_t* genSizes = computeHeapsizes(heapsize);
	for (i = 0; i < GENERATIONS; i++) {
		switch (_allocator) {
			case basicAlloc:
				myAllocators[i] = new BasicAllocator();
				myAllocators[i]->initializeHeap(genSizes[i]);
				break;
			case nextFitAlloc:
				myAllocators[i] = new NextFitAllocator();
				myAllocators[i]->initializeHeap(genSizes[i]);
				break;
			case regionBased:
			case threadBased:
				myAllocators[i] = new RegionBasedAllocator();
				myAllocators[i]->initializeHeap(genSizes[i], maxheapsize);
				break;
		}
	}
	free(genSizes);
}

void MemoryManager::initContainers() {
	int i;
	for (i = 0; i < GENERATIONS; i++) {
		myObjectContainers[i] = new ObjectContainer();
	}
}

/** Sets the garbage collector as the specified concrete implementation
 * and calls it's
 * Collector::setEnvironment(Allocator,ObjectContainer,MemoryManager,int,int,int)
 * and Collector::initializeHeap() member functions
 *
 * @param highWatermark
 */
void MemoryManager::initGarbageCollectors(size_t highWatermark) {
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
			case balanced:
			case markSweepTB:
				myGarbageCollectors[i] = new BalancedCollector();
				break;
		}
		myGarbageCollectors[i]->setEnvironment(myAllocators[i],	myObjectContainers[i], this, highWatermark, i, _traversal);
		myGarbageCollectors[i]->initializeHeap();
	}
}

/** Sets the write barrier to the correct concrete implementation
 *
 */
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

/** Collects and promotes objects to make enough room in the nursery for
 * an object of specified size. Crashes simulator with error message if
 * promotion & collection cannot free up enough space. Allocates the object if
 * enough space is made in the nursery.
 *
 * @param size size of the object to be allocated
 * @return address of the allocated object
 */
void *MemoryManager::shift(size_t size){
	void *result = NULL;
	int outOfMemory = 0;
	size_t spaceOnTop = myAllocators[GENERATIONS-1]->getFreeSize();
	while(result == NULL && spaceOnTop >= size){
#if WRITE_DETAILED_LOG==1
			fprintf(gDetLog,"(" TRACE_FILE_LINE_FORMAT ") SHIFTING for %zu\n",gLineInTrace,size);
#endif
		myGarbageCollectors[GENERATIONS-1]->collect((int)reasonShift);
		outOfMemory = myGarbageCollectors[GENERATIONS-1]->promotionPhase();
		if(outOfMemory==-1){
			fprintf(stderr,"(" TRACE_FILE_LINE_FORMAT ") OUT OF MEMORY: (%zu)\n",gLineInTrace,size);
			exit(1);
		}
		result = myAllocators[0]->gcAllocate(size);
	}
	return result;
}

/** Attempt to allocate an object in specific generation.
 * If there is not enough space in the generation, collects
 * and promotes objects until either allocation is successful
 * or all generations have been collected and promoted.
 * @param size The size of the object to allocate
 * @param generation The generations to allocate to [0, GENERATIONS)
 * @return The address of the allocated object, or NULL if allocation fails
 */
void *MemoryManager::allocate(size_t size, int generation) {
	//check if legal generation
	if (generation < 0 || generation > GENERATIONS - 1) {
		fprintf(stderr, "ERROR (Line " TRACE_FILE_LINE_FORMAT "): allocate to illegal generation: %d\n",
				gLineInTrace, generation);
		exit(1);
	}
	void *result = NULL;
	int gen = generation;
	//try allocating in the generation
	result = myAllocators[generation]->gcAllocate(size);
	while (result == NULL && gen < GENERATIONS) {
#if WRITE_DETAILED_LOG == 1
		fprintf(gDetLog,
				"(" TRACE_FILE_LINE_FORMAT ") Trigger Gc in generation %d.\n",
				gLineInTrace, gen);
#endif

		myGarbageCollectors[gen]->collect(reasonFailedAlloc);

		result = myAllocators[generation]->gcAllocate(size);

		gen++;
	}
	if (gen > generation) {
		//gcs were made. promote if possible
		myGarbageCollectors[gen - 1]->promotionPhase();
	}
#if GENERATIONS > 1 && SHIFTING == 1
	if(result == NULL){
		result = shift(size);
	}
#endif
	return result;
}

/*void *MemoryManager::allocate(size_t size, int generation, int thread) { //if region-based; by Tristan
	//check if legal generation
	if (generation < 0 || generation > GENERATIONS - 1) {
		fprintf(stderr, "ERROR (Line %lld): allocate to illegal generation: %d\n",
				gLineInTrace, generation);
		exit(1);
	}
	void *result = NULL;
	int gen = generation;
	//try allocating in the generation
	result = myAllocators[generation]->gcAllocate(size,thread);
	while ((long)result < 0 && gen < GENERATIONS) {
#if WRITE_DETAILED_LOG == 1
		fprintf(gDetLog,
				"(%lld) Trigger Gc in generation %d.\n",
				gLineInTrace, gen);
#endif

		result = myAllocators[generation]->gcAllocate(size);

		gen++;
	}
	if (gen > generation) {
		//gcs were made. promote if possible
		myGarbageCollectors[gen - 1]->promotionPhase();
	}

#if GENERATIONS > 1 && SHIFTING == 1
	if(result == NULL){
		result = shift(size);
	}
#endif

	return result;
}
*/

/** Add object to correct container and add reference to
 *  remember sets where required.
 *
 * @param object Object to be added to a container
 * @param thread ID of the thread adding the object
 */
void MemoryManager::addRootToContainers(Object* object, int thread) {

	int i;
	for (i = 0; i < GENERATIONS; i++) {
		if (i == GENERATIONS - 1) {
			myObjectContainers[i]->addToRoot(object, thread);
		} //otherwise if there is more than one generation, add new object to remSets
		else {
			myObjectContainers[i]->add(object);
			myObjectContainers[i]->addToGenRoot(object);
#if WRITE_DETAILED_LOG == 1
			fprintf(gDetLog, "(" TRACE_FILE_LINE_FORMAT ") Adding %d to remset %d\n", gLineInTrace,
					object->getID(), i);
#endif
		}
	}
}

/** Checks if the object is larger than a region.
 * 	If so and it is an array, allocates an arraylet
 * 	If so and not an array then undefined behavior
 * 	If not, then regular allocation continues
 *
 * @param thread Thread ID of allocating thread
 * @param id Object ID
 * @param size Object size in bytes
 * @param refCount Number of references the object contains
 * @param classID Class ID for the object
 * @return 0
 */
int MemoryManager::preAllocateObjectDefault(int thread, int id, size_t size, size_t refCount, int classID) {
#if WRITE_DETAILED_LOG == 1
	fprintf(gDetLog, "(" TRACE_FILE_LINE_FORMAT ") Add Root to thread %d with id %d\n", gLineInTrace, thread, id);
#endif
	void *address = NULL;
	size_t regionSize = myAllocators[GENERATIONS-1]->getRegionSize();

	if(size > regionSize){ //If object won't fit in a region, check if it can be allocated as arraylet
		if(isArray(classID) && _collector == balanced){ //Can allocate as an arraylet
			size_t spineSize, fullLeaves, numberOfObjectsPerLeaf, finalLeafObjects, lastLeaf, leaves;

			if(refCount == 0){
				numberOfObjectsPerLeaf = 0;
				fullLeaves = size/regionSize;
				finalLeafObjects = 0;
				lastLeaf = size%regionSize;
				leaves = (lastLeaf > 0 ? fullLeaves + 1 : fullLeaves);
				spineSize = sizeof(void*) + sizeof(Object*) + leaves * sizeof(RawObject*);
			}
			else{
				numberOfObjectsPerLeaf = (regionSize/sizeof(RawObject*)) - 1;
				fullLeaves = refCount/numberOfObjectsPerLeaf;
				finalLeafObjects = refCount - numberOfObjectsPerLeaf*fullLeaves;
				leaves = (finalLeafObjects > 0 ? fullLeaves + 1 : fullLeaves);
				spineSize = sizeof(void*) + sizeof(Object*) + leaves * sizeof(RawObject*);
				lastLeaf = finalLeafObjects*sizeof(RawObject*) + sizeof(RawObject*);
			}

			if(spineSize > regionSize){ //Spine does not fit in one region
				fprintf(stderr, "[MemoryManager::preAllocateObjectDefault][arraylet] The final spine allocation size must fit into a region. spineSize (%zu) > regionSize(%zu)\n", spineSize, regionSize);
				return postAllocateObjectToRootset(thread,id,size,refCount,classID,NULL); //call for consitent error message
			}
			address = allocate(spineSize, 0);
			if(address == NULL){
				return postAllocateObjectToRootset(thread,id,size,refCount,classID,NULL); //call for consitent error message
			}
			Object *spine = new Object(id, address, spineSize, leaves, getClassName(classID), allocationTypeDiscontiguousIndexable);
			spine->setGeneration(0);
			addRootToContainers(spine, thread);
				//Allocate leaves
			size_t i;
			for(i = 0; i < fullLeaves; i++){
				address = allocate(regionSize, 0);
				if(address == NULL){
					return postAllocateObjectToRootset(thread,id,size,refCount,classID,NULL); //call for consitent error message
				}
				Object *leaf = new Object(-1, address, regionSize, numberOfObjectsPerLeaf, &getClassName(classID)[1],allocationTypeDiscontiguousIndexable);
				spine->setPointer(i, leaf);
				myAllocators[GENERATIONS-1]->getRegions()[myAllocators[GENERATIONS-1]->getObjectRegion(leaf)]->setIsLeaf(true); //Mark as leaf region
			}
			//Allocate final leaf
			if(lastLeaf > 0){
				address = allocate(lastLeaf, 0);
				if(address == NULL){
					return postAllocateObjectToRootset(thread,id,size,refCount,classID,NULL); //call for consitent error message
				}
				Object *leaf = new Object(-1, address, lastLeaf, finalLeafObjects, &getClassName(classID)[1],allocationTypeDiscontiguousIndexable);
				spine->setPointer(i, leaf);
				myAllocators[0]->getRegions()[myAllocators[GENERATIONS-1]->getObjectRegion(leaf)]->insertObjectReference(spine->getAddress()); //Add remset pointer
			}
			printArraylet(spine, size);
			return 0;
		}
		else{
			//TODO: Resize or Merge regions?
			fprintf(stderr, "ERROR: Object of size %zu (Line: " TRACE_FILE_LINE_FORMAT ") is too large for a single region of size %zu.\n", size, gLineInTrace, regionSize);
			fprintf(gLogFile, "ERROR: Object of size %zu (Line: " TRACE_FILE_LINE_FORMAT ") is too large for a single region of size %zu.\n", size, gLineInTrace, regionSize);
			fclose(gLogFile);

			if(_collector == balanced)
			{
				fprintf(balancedLogFile, "ERROR: Object of size %zu (Line: " TRACE_FILE_LINE_FORMAT ") is too large for a single region of size %zu.\n", size, gLineInTrace, regionSize);
				fclose(balancedLogFile);
			}

			throw 19;
		}
	}
	else{ //Object can fit in region, allocate as normal
		address = allocate(size, 0);
	}
	return postAllocateObjectToRootset(thread,id,size,refCount,classID,address);
}

/** Attempts to allocate space for an object from a given thread.
 *
 * @param thread Thread ID of allocating thread
 * @param id Object ID
 * @param size Object size in bytes
 * @param refCount Number of references the object contains
 * @param classID Class ID for the object
 * @return 0
 */
int MemoryManager::preAllocateObjectThreadBased(int thread, int id, size_t size, size_t refCount, int classID) {
	int headObj, parentID;
	int objID;
	size_t regionSize, remainingSize, parentSlot, newRefCount;
	void* address;

	headObj = 1;
	newRefCount = refCount;
	objID = id;
	remainingSize = size;
	regionSize = myAllocators[GENERATIONS-1]->getRegionSize();

	while (remainingSize > regionSize) { //if object size is > region size, divide object into several chunks (objects) or arraylet
		address = allocate(regionSize, 0);

		if (headObj) {
			newRefCount = refCount+1;
		}
		else { //if trailing chunk
			objID = --chunkID;
			newRefCount = 1;
		}

		postAllocateObjectToRootset(thread,objID,regionSize,newRefCount,classID,address);

		if (!headObj) { //if trailing chunk
			regionSetPointer(thread,parentID,parentSlot,objID);					//point previous chunk to the current chunk
			myObjectContainers[GENERATIONS - 1]->removeFromRoot(thread, objID);	//if trailing chunk, remove from root
		}
		else
			headObj = 0;

		parentID = objID; //current Object will becom the parent of the next object in the chunk list
		parentSlot = newRefCount-1; //parent slot will point to the next object in the chunk list

		remainingSize -= regionSize;
	}

	if (remainingSize > 0) { //allocates the object if (remaining) size is less than region Size, otherwise allocates the object itself
		if (remainingSize < 24) //minimum object size;
			remainingSize = 24;

		address = allocate(remainingSize, 0);
		if (!headObj) {
			objID = --chunkID;
			newRefCount = 0;
		}

		postAllocateObjectToRootset(thread,objID,remainingSize,newRefCount,classID,address);

		if (!headObj) { //if trailing chunk
			regionSetPointer(thread,parentID,parentSlot,objID); //point parent to the current object
			myObjectContainers[GENERATIONS - 1]->removeFromRoot(thread, objID); //if trailing arraylet, remove from root
		}
	}
	return 0;
}

/** Delegate method for calling MemoryManager::preAllocateObject(int,int,size_t,int,int)
 *
 * @param thread Thread ID of allocating thread
 * @param id Object ID
 * @param size Object size in bytes
 * @param refCount Number of references the object contains
 * @param classID Class ID for the object
 * @return 0
 */
int MemoryManager::allocateObjectToRootset(int thread, int id, size_t size, size_t refCount, int classID) {

	(*this.*preAllocateObject)(thread,id,size,refCount,classID);

	return 0;
}

/** Checks if an allocation for the requested size succeeded, and if so
 * creates the Object structure and adds it to it's container.
 *
 * @param thread Thread ID of allocating thread
 * @param id Object ID
 * @param size Object size in bytes
 * @param refCount Number of references the object contains
 * @param classID Class ID for the object
 * @param address
 * @return 0
 */
inline int MemoryManager::postAllocateObjectToRootset(int thread, int id,size_t size, size_t refCount, int classID,void *address) {//post allocation; by Tristan
	if (address == NULL) {
		fprintf(gLogFile, "Failed to allocate %zu bytes in trace line " TRACE_FILE_LINE_FORMAT ".\n",size, gLineInTrace);
		fprintf(stderr, "ERROR(Line " TRACE_FILE_LINE_FORMAT "): Out of memory (%zu bytes)\n",gLineInTrace,size);
		myGarbageCollectors[GENERATIONS-1]->lastStats();
		throw 19;
	}
	//create Object
	Object *object;
	object = new Object(id, address, size, refCount, getClassName(classID), allocationTypeObject);

	object->setGeneration(0);
	//add to Containers
	addRootToContainers(object, thread);

	if (myWriteBarrier){
		myWriteBarrier->process(NULL, object);
	}

#if DEBUG_MODE == 1
	myGarbageCollectors[GENERATIONS - 1]->collect(reasonDebug);
	myGarbageCollectors[GENERATIONS - 1]->promotionPhase();
#endif

	return 0;
}

/** Removes an object from it's root, and from all remember sets.
 * If there is a write barrier then in will also process the roots
 *
 * @param thread Thread ID or removing thread
 * @param id Object ID
 * @return 0
 */
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
#if ZOMBIE == 1
		myGarbageCollectors[0]->collect(reasonFailedAlloc);
#endif
	}

	return 0;
}

bool MemoryManager::isAlreadyRoot(int thread, int id) {
	return myObjectContainers[GENERATIONS-1]->isAlreadyRoot(thread, id);
}

/** Add's the object to the root container. If there is a write barrier
 * then process the write.
 *
 * @param thread Thread ID of adding thread
 * @param id Object ID
 * @return 0
 */
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

/** Removes all references to an object, starting at it's generation until
 * the tenure generation. Cleans leaflets if the object is an Arraylet.
 * Frees allocators reference then deletes object.
 *
 * @param object Object to be removed.
 * @param gGC Unused parameter
 */
void MemoryManager::requestDelete(Object* object, int gGC) {
#if WRITE_DETAILED_LOG == 1
	fprintf(gDetLog, "(" TRACE_FILE_LINE_FORMAT ") Delete object with id %d\n", gLineInTrace,
			object->getID());
#endif
	int i;
	int objGeneration = object->getGeneration();

	//delete object from all Gen Roots it might be in
	for (i = objGeneration + 1; i < GENERATIONS; i++) {
		int status = myObjectContainers[i]->removeReferenceTo(object);
		if (status == -1) {
			fprintf(stderr, "ERROR(Line " TRACE_FILE_LINE_FORMAT "):Object %d(g%d) could not be removed from object container %d\n", gLineInTrace, object->getID(), objGeneration, i);
		}
	}

	//if this is an arraylet we have to also clean the leafs
	//TODO: this shouldn't be here, leaves should be freed in collector when spine is dead
	if(object->allocationType == allocationTypeDiscontiguousIndexable){
		size_t spineMaxPointers = object->getPointersMax();
		Object *leaf;
		size_t leafRegion;
		std::vector<Region*> myAllRegions = myAllocators[0]->getRegions();
		//unsigned char * heapStartAddress = myAllRegions[0]->getHeapAddress();
		//unsigned int regionSize = myAllocators[0]->getRegionSize();

		for(size_t idx = 0; idx < spineMaxPointers; idx++){
			leaf = object->getReferenceTo(idx);
			if(leaf){
				leafRegion = myAllocators[0]->getObjectRegion(leaf);//((char *)leaf->getAddress() - (char *)heapStartAddress) / regionSize;
				//double checking if the region is an arraylet leaf
				if(myAllRegions[leafRegion]->getIsLeaf()){
					//this free operations are from balancedcollector::reOrganaizeRegions
					myAllRegions[leafRegion]->reset();
					myAllocators[0]->setRegionFree(myAllocators[0]->getRegions()[leafRegion]);
					myAllocators[0]->addNewFreeRegion(leafRegion);
				}
			}
		}
	}

	//now free in allocator and delete object
	myAllocators[objGeneration]->gcFree(object);
	myObjectContainers[objGeneration]->deleteObject(object, !myAllocators[objGeneration]->isRealAllocator());
}

/** Moves an object to the next generation if possible.
 *  Must remove old remember set entries and add new ones after moving.
 *
 * @param object Object structure containing information about object to be moved
 * @return 0 on success, 1 on failure
 */
int MemoryManager::requestPromotion(Object* object) {
	if (object->getGeneration() == GENERATIONS - 1) {
#if WRITE_DETAILED_LOG == 1
		fprintf(gDetLog,"(" TRACE_FILE_LINE_FORMAT ") Request to promote %d, but as it is in maxGen, not granted.\n",gLineInTrace, object->getID());
#endif
		return 0;
	}

	int oldGen = object->getGeneration();
	int newGen = oldGen + 1;
	size_t size = object->getHeapSize();

#if WRITE_DETAILED_LOG == 1
	fprintf(gDetLog, "(" TRACE_FILE_LINE_FORMAT ") Request to promote %d from %d to %d\n",gLineInTrace, object->getID(), oldGen, newGen);
#endif

	void *address = myAllocators[newGen]->gcAllocate(size);
	memcpy(address, object->getAddress(), size);
	if (address == NULL) {
		//there is not enough space upstairs, stay where you are for a little longer
#if WRITE_DETAILED_LOG == 1
		fprintf(gDetLog,"(" TRACE_FILE_LINE_FORMAT ") Request to promote %d from %d to %d not possible (no space)\n",gLineInTrace, object->getID(), oldGen, newGen);
#endif
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
#if WRITE_DETAILED_LOG == 1
		fprintf(gDetLog,"(" TRACE_FILE_LINE_FORMAT ") Removing myself %d from remset %d (promotion))\n",gLineInTrace, object->getID(), oldGen);
#endif
	}
	//handle children
	size_t i;
	for (i = 0; i < object->getPointersMax(); i++) {
		Object* child = object->getReferenceTo(i);
		if (child && child->getGeneration() == oldGen) {
			myObjectContainers[oldGen]->addToGenRoot(child);
#if WRITE_DETAILED_LOG == 1
			fprintf(gDetLog,"(" TRACE_FILE_LINE_FORMAT ") Adding %d to remset %d (parent (%d) was promoted))\n",gLineInTrace, child->getID(), oldGen, object->getID());
#endif
		}
	}
	return 0;
}

void MemoryManager::requestReallocate(Object* object) {
#if WRITE_DETAILED_LOG == 1
//		fprintf(gDetLog, "(%d) Reallocate request for id %d\n", gLineInTrace,
//				object->getID());
#endif

	if (object) {
		int gen = object->getGeneration();
		size_t size = object->getHeapSize();
		void *address = myAllocators[gen]->gcAllocate(size);
		memcpy(address, object->getAddress(), size);
		if (address == NULL) {
			fprintf(stderr,"ERROR(Line %lld):Could not reallocate Object %d to gen %d\n",gLineInTrace, object->getID(), gen);
			exit(1);
		}
		object->updateAddress(address);
		//TODO what about the old RawObject? How does it get freed?
		// In markSweep collection, the entire heap is explicitly freed (but no
		// objects are deleted) during the compaction phase.

		//object->setFreed(0);

	}
}

/** Determines Parent Type and call appropriate delegate function.
 * Delegate function(s) set the reference in a parent to point to the child object.
 * Updates remember set entries based on new pointer reference setting.
 *
 * @param thread ID of thread calling set pointer
 * @param parentID ID of parent object
 * @param parentSlot Slot number to set
 * @param childID ID of object to be referenced from the parent object
 * @return
 */
int MemoryManager::setPointer(int thread, int parentID, size_t parentSlot, int childID)
{
	parent = myObjectContainers[GENERATIONS - 1]->getByID(parentID);

	if(parent->allocationType == allocationTypeObject)
	{
		return setObjectPointer(thread, parentID, parentSlot, childID);
	}
	else if(parent->allocationType == allocationTypeDiscontiguousIndexable)
	{
		return setArrayletPointer(thread, parentID, parentSlot, childID);
	}
	else
	{
		fprintf(stderr, "Write operation on non-supported object type \nsetPointer function of MemoryManager.cpp");
		throw 19;
	}
}

/** Sets the reference in a parent object to point to the child object.
 * Updates remember set entries based on new pointer reference setting.
 *
 * @param thread ID of thread calling set pointer
 * @param parentID ID of parent object
 * @param parentSlot Slot number to set
 * @param childID ID of object to be referenced from the parent object
 * @return
 */
int MemoryManager::setObjectPointer(int thread, int parentID, size_t parentSlot, int childID)
{
	int parentGeneration;

#if WRITE_DETAILED_LOG == 1
	fprintf(gDetLog, "(" TRACE_FILE_LINE_FORMAT ") Set object pointer from %d(%d) to %d\n", gLineInTrace,parentID, parentSlot, childID);
#endif

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
			ss << "Child object " << childID << " does not exist. Ignoring trace file statement and continuing.\n"; //TODO: shouldn't there be an exit/ return failure?
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
		ss << "Parent object " << parentID << " does not exist. Ignoring trace file statement and continuing.\n"; //TODO: shouldn't there be an exit/ return failure?
		ERRMSG(ss.str().c_str());
		return 0;
	}

	//check old child, if it has remSet entries then delete them
	if (oldChild && parentGeneration > oldChild->getGeneration()) {
		int i;
		for (i = oldChild->getGeneration(); i < parentGeneration; i++) {
#if WRITE_DETAILED_LOG == 1
			fprintf(gDetLog,"(" TRACE_FILE_LINE_FORMAT ") removing %d from remset %d (i am oldchild of (%d) in setObjectPointer)\n",gLineInTrace, child->getID(), i, parent->getID());
#endif
			int status = myObjectContainers[i]->removeFromGenRoot(oldChild);
			if (status == -1) {
				fprintf(stderr,"ERROR (Line " TRACE_FILE_LINE_FORMAT "): could not remove oldChild %d from remset %d\n",gLineInTrace, oldChild->getID(), i);
				//TODO: shouldn't there be an exit/ return failure?
			}
		}
	}

	if (parent){
		if(parent->allocationType == allocationTypeObject){
			parent->setPointer(parentSlot, child);
		}
		else {
			fprintf(stderr, "Write operation on non-supported object type \nsetObjectPointer function of MemoryManager.cpp");
			throw 19;
		}
	}

	if (parentGeneration > childGeneration && childID != 0) {
		int i;
		for (i = childGeneration; i < parentGeneration; i++) {
			if (child)
				myObjectContainers[i]->addToGenRoot(child);
#if WRITE_DETAILED_LOG == 1
			fprintf(gDetLog,"(" TRACE_FILE_LINE_FORMAT ") Adding %d to remset %d (parent (%d) got a new pointer to me))\n",gLineInTrace, child->getID(),i, parent->getID());
#endif
		}
	}
#if DEBUG_MODE == 1
	myGarbageCollectors[GENERATIONS - 1]->collect(reasonDebug);
	myGarbageCollectors[GENERATIONS - 1]->promotionPhase();
#endif

	if (myWriteBarrier) {
		myWriteBarrier->process(oldChild, child);
#if ZOMBIE == 1
		myGarbageCollectors[0]->collect(reasonFailedAlloc);
#endif
	}

	return 0;
}

/** Sets the reference in a parent arraylet to point to the child object.
 * Updates remember set entries based on new pointer reference setting.
 *
 * @param thread ID of thread calling set pointer
 * @param parentID ID of parent object
 * @param parentSlot Slot number to set
 * @param childID ID of object to be referenced from the parent object
 * @return
 */
int MemoryManager::setArrayletPointer(int thread, int parentID, size_t parentSlot, int childID)
{
	int parentGeneration;

	size_t leafMaxPointers = parent->getReferenceTo(0)->getPointersMax();
	size_t leafSlot = parentSlot % leafMaxPointers;
	size_t spineSlot = parentSlot / leafMaxPointers;

#if WRITE_DETAILED_LOG == 1
	fprintf(gDetLog, "(" TRACE_FILE_LINE_FORMAT ") Set arraylet pointer from %d(%d) to %d\n", gLineInTrace,parentID, parentSlot, childID);
#endif

	child = NULL;
	int childGeneration = -1;
	if(childID != 0){
		child = myObjectContainers[GENERATIONS - 1]->getByID(childID);
		if (child)
			childGeneration = child->getGeneration();
		else
			fprintf(stderr, "Child %i not existing in setArrayletPointer\n", childID);
	}

	if (parent){
		parentGeneration = parent->getGeneration();
	}
	else{
		fprintf(stderr, "Parent %i not existing in setArrayletPointer\n", parentID);
	}

	//check old child, if it created remSet entries and delete them
	if (parent){
		oldChild = parent->getReferenceTo(spineSlot)->getReferenceTo(leafSlot);
	}
	if (oldChild && parentGeneration > oldChild->getGeneration()){
		int i;
		for (i = oldChild->getGeneration(); i < parentGeneration; i++){
#if WRITE_DETAILED_LOG == 1
			fprintf(gDetLog,"(" TRACE_FILE_LINE_FORMAT ") removing %d from remset %d (i am oldchild of (%d) in setArrayletPointer)\n",gLineInTrace, child->getID(), i, parent->getID());
#endif
			int status = myObjectContainers[i]->removeFromGenRoot(oldChild);
			if (status == -1)
			{
				fprintf(stderr,"ERROR (Line " TRACE_FILE_LINE_FORMAT "): could not remove oldChild %d from remset %d\n",gLineInTrace, oldChild->getID(), i);
				//TODO: shouldn't there be an exit/ return failure?
			}
		}
	}

	if (parent){
		if(parent->allocationType == allocationTypeDiscontiguousIndexable){
			parent->getReferenceTo(spineSlot)->setPointer(leafSlot, child);
		}
		else{
			fprintf(stderr, "Write operation on non-supported object type \nsetArrayletPointer function of MemoryManager.cpp");
			//TODO: shouldn't there be an exit/ return failure?
		}
	}

	if (parentGeneration > childGeneration && childID != 0){
		int i;
		for (i = childGeneration; i < parentGeneration; i++){
			if (child)
				myObjectContainers[i]->addToGenRoot(child);
#if WRITE_DETAILED_LOG == 1
			fprintf(gDetLog,"(" TRACE_FILE_LINE_FORMAT ") Adding %d to remset %d (parent (%d) got a new pointer to me))\n",gLineInTrace, child->getID(),i, parent->getID());
#endif
		}
	}
#if DEBUG_MODE == 1
	myGarbageCollectors[GENERATIONS - 1]->collect(reasonDebug);
	myGarbageCollectors[GENERATIONS - 1]->promotionPhase();
#endif

	if (myWriteBarrier)
	{
		myWriteBarrier->process(oldChild, child);
#if ZOMBIE == 1
		myGarbageCollectors[0]->collect(reasonFailedAlloc);
#endif
	}

	return 0;
}

/** Calls set pointer and adds a reference to the parent object
 * from the child's region.
 *
 * @param thread ID of thread setting the parent's reference
 * @param parentID ID of the parent object
 * @param parentSlot Slot being set
 * @param childID ID of child object being set in the reference
 * @return 0
 */
int MemoryManager::regionSetPointer(int thread, int parentID, size_t parentSlot,int childID) {
	size_t parentRegion,childRegion;

	setPointer(thread,parentID,parentSlot,childID);

	parentRegion = myAllocators[0]->getObjectRegion(parent);

	if (child) {
		childRegion = myAllocators[0]->getObjectRegion(child);

		if (parentRegion != childRegion) {
			myAllocators[0]->getRegions()[childRegion]->insertObjectReference(parent->getAddress());
			//fprintf(balancedLogFile, "Added remset entry from parent %i in region %zu to child %i in region %zu\n", parentID, parentRegion, childID, childRegion);
		}
	}

	return 0;
}

/** Sets a static reference to point to the location of an object.
 *
 * @param classID Class containing the static field
 * @param fieldOffset offset of the field into class
 * @param objectID ID of the object to be referenced
 */
void MemoryManager::setStaticPointer(int classID, int fieldOffset, int objectID) {
	Object* myChild;
	Object* myOldChild;

	myOldChild = myObjectContainers[GENERATIONS - 1]->getStaticReference(classID, fieldOffset);

	myObjectContainers[GENERATIONS - 1]->setStaticReference(classID, fieldOffset, objectID);

	if (myWriteBarrier) {
		myChild = myObjectContainers[GENERATIONS - 1]->getStaticReference(classID, fieldOffset);
		myWriteBarrier->process(myOldChild, myChild);
#if ZOMBIE == 1
		myGarbageCollectors[0]->collect(reasonFailedAlloc);
#endif
	}
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
	for(i= objGen ; i < GENERATIONS-1; i++){
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
			result[i] = (size_t)ceil(heapLeft * (1.0 - GENRATIO)); //no byte is left behind
			heapLeft = (size_t)(heapLeft * GENRATIO);
		}
#if GEN_DEBUG == 1
		printf("GENDEBUG: G%d: %zu\n", i, result[i]);
#endif
	}

#if GEN_DEBUG == 1
	size_t sum = 0;
	for (i = 0; i < GENERATIONS; i++) {
		sum = sum + result[i];
	}
	printf("GENDEBUG: Sum of Generations: %zu\n", sum);
#endif

	return result;
}

void MemoryManager::printStats() {
	for(int i=0; i < GENERATIONS;i++){
		myGarbageCollectors[i]->printStats();
	}

}

/** Assumes given object is an arraylet and prints information
 * about the arraylet to the standard output.
 *
 * @param arraylet
 * @param size
 */
void MemoryManager::printArraylet(Object* arraylet, size_t size){
	//arraylets are currently only implemented in the balanced collector
	if(_collector == balanced){
		size_t regionSize = myAllocators[GENERATIONS-1]->getRegionSize();
		size_t numberOfFullLeaves = size / regionSize;
		size_t leftoverSize = size % regionSize;
		RawObject* rawSpine = (RawObject*)arraylet->getAddress();

		std::cout<<"---------"<<std::endl;
		std::cout<<"size: "<<size<<std::endl;
		std::cout<<"regionSize: "<<regionSize<<std::endl;
		std::cout<<"leftoversize: "<<leftoverSize<<std::endl;
		std::cout<<"#ofleafs: "<<numberOfFullLeaves<<std::endl;
		std::cout<<"spine: "<<arraylet<<std::endl;
		std::cout<<"rawSpine: "<<rawSpine<<std::endl;
		std::cout<<"---------"<<std::endl;
		for(size_t idx=0;idx<numberOfFullLeaves;idx++){
			std::cout<<"#ofLeaf: "<<idx<<" getRef: "<<arraylet->getReferenceTo(idx)<<" size: "<<regionSize<<std::endl;
		}

		if(leftoverSize != 0){
			numberOfFullLeaves++;
			std::cout<<"leftover:"<<std::endl<<"#ofLeaf: "<<numberOfFullLeaves-1<<" getRef: "<<arraylet->getReferenceTo(numberOfFullLeaves-1)<<" size: "<<leftoverSize<<std::endl;
		}
		std::cout<<"---------"<<std::endl;
	}
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
