/*
 * MemoryManager.hpp
 *
 *  Created on: 2013-09-03
 *      Author: GarCoSim
 */

#ifndef MEMORYMANAGER_HPP_
#define MEMORYMANAGER_HPP_


#include "../defines.hpp"
#include <vector>
#include <string>

using namespace std;

namespace traceFileSimulator {
class Object;
class Allocator;
class Collector;
class ObjectContainer;
class WriteBarrier;

class MemoryManager {
public:
	MemoryManager(size_t heapSize, size_t maxHeapSize, int highWatermark, int collector, int traversal, int allocator, int writebarrier);
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
	void initAllocators(size_t heapsize, size_t maxheapsize);
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

};

}
#endif /* MEMORYMANAGER_HPP_ */
