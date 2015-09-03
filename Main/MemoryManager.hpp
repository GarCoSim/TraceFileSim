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
#include "../Collectors/Collector.hpp"
#include "../Collectors/MarkSweepCollector.hpp"
#include "../Collectors/TraversalCollector.hpp"
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
	MemoryManager(int heapSize, int highWatermark, int collector, int traversal, int allocator);
	virtual ~MemoryManager();
	//operations possible from trace file
	int allocateObjectToRootset(int thread, int id, int size, int refCount, int classID);
	int requestRootDelete(int thread, int id);
	int requestRootAdd(int thread, int id);
	int setPointer(int thread, int parentID, int parentSlot, int childID);
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
	int* computeHeapsizes(int heapSize);
	void initAllocators(int heapsize);
	void initContainers();
	void initGarbageCollectors(int highWatermark);
	void *allocate(int size, int generation);
	void addRootToContainers(Object* object, int thread);
	void addToContainers(Object* object);
	void *shift(int size);
	
	allocatorEnum _allocator;
	collectorEnum _collector;
	traversalEnum _traversal;

	bool classTableLoaded;
	vector<string> classTable;

	Allocator* myAllocators[GENERATIONS];
	ObjectContainer* myObjectContainers[GENERATIONS];
	Collector* myGarbageCollectors[GENERATIONS];
	int stats[GENERATIONS];

};

} 
#endif /* MEMORYMANAGER_HPP_ */
