/*
 * MemoryManager.hpp
 *
 *  Created on: 2013-09-03
 *      Author: GarCoSim
 */

#ifndef MEMORYMANAGER_HPP_
#define MEMORYMANAGER_HPP_

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
#include "../defines.hpp"
#include <string>
#include <vector>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

using namespace std;

namespace traceFileSimulator {

class MemoryManager {
public:
	MemoryManager(size_t heapSize, int highWatermark, int collector, int traversal, int allocator, int writebarrier);
	virtual ~MemoryManager();
	//operations possible from trace file
	int allocateObjectToRootset(int thread, int id, size_t size, int refCount, int classID);
	int requestRootDelete(int thread, int id);
	int requestRootAdd(int thread, int id);
	int setPointer(int thread, int parentID, int parentSlot, int childID);
	void setStaticPointer(int classID, int fieldOffset, int objectID);
	void requestDelete(Object* object, int gGC);
	void requestFree(Object* object);
	void requestReallocate(Object* object);
	void requestResetAllocationPointer(int generation);
	int requestPromotion(Object* object);
	void printStats();
	void statBeforeCompact(int myGeneration);
	void statAfterCompact(int myGeneration);
	int evalCollect();
	void createRemSetEntries(Object* parent, Object* child);
	void createRemSetEntriyRoot(Object* object);
	void clearRemSets();
	void requestRemSetAdd(Object* currentObj);
	char *getClassName(int classNumber);
	bool loadClassTable(string traceFilePath);
	bool hasClassTable();
	void forceGC();
	void lastStats();
	void dumpHeap();


private:
	bool isAlreadyRoot(int thread, int id);
	size_t* computeHeapsizes(size_t heapSize);
	void initAllocators(size_t heapsize);
	void initContainers();
	void initGarbageCollectors(int highWatermark);
	void initWritebarrier();
	void *allocate(size_t size, int generation);
	void addRootToContainers(Object* object, int thread);
	void addToContainers(Object* object);
	void *shift(size_t size);
	
	allocatorEnum _allocator;
	collectorEnum _collector;
	traversalEnum _traversal;
	writebarriersEnum _writebarrier;

	bool classTableLoaded;
	vector<string> classTable;

	Allocator* myAllocators[GENERATIONS];
	ObjectContainer* myObjectContainers[GENERATIONS];
	Collector* myGarbageCollectors[GENERATIONS];
	WriteBarrier* myWriteBarrier;
	int stats[GENERATIONS];
	Object *parent,*child,*oldChild;
};

} 
#endif /* MEMORYMANAGER_HPP_ */
