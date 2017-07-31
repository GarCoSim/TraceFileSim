/*
 * MemoryManager.hpp
 *
 *  Created on: 2013-09-03
 *      Author: GarCoSim
 */

#ifndef MEMORYMANAGER_HPP_
#define MEMORYMANAGER_HPP_


#include "../defines.hpp"
#include <map>
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
	int allocateObjectToRootset(int thread, int id, size_t size, size_t refCount, int classID);
	inline int postAllocateObjectToRootset(int thread, int id,size_t size, size_t refCount, int classID,void *address);
	int requestRootDelete(int thread, int id);
	int requestRootAdd(int thread, int id);
	void requestReallocate(Object* object);
	int setPointer(int thread, int parentID, size_t parentSlot, int childID);
	int setObjectPointer(int thread, int parentID, size_t parentSlot, int childID);
	int setArrayletPointer(int thread, int parentID, size_t parentSlot, int childID);
	int regionSetPointer(int thread, int parentID, size_t parentSlot,int childID);
	void setStaticPointer(int classID, int fieldOffset, int objectID);
	void requestDelete(Object* object, int gGC);
	int requestPromotion(Object* object);
	void printStats();
	void printArraylet(Object* arraylet, size_t size);
	void clearRemSets();
	void requestRemSetAdd(Object* currentObj);
	char *getClassName(int classId);
	bool isArray(int classID);
	bool loadClassTable(string traceFilePath);
	bool hasClassTable();
	void forceGC();
	void lastStats();


private:
	bool isAlreadyRoot(int thread, int id);
	size_t* computeHeapsizes(size_t heapSize);
	void initAllocators(size_t heapsize, size_t maxheapsize);
	void initContainers();
	void initGarbageCollectors(size_t highWatermark);
	void initWritebarrier();
	void *allocate(size_t size, int generation);
	//void *allocate(size_t size, int generation, int thread);
	void addRootToContainers(Object* object, int thread);
	void *shift(size_t size);

	int preAllocateObjectDefault(int thread, int id, size_t size, size_t refCount, int classID);
	int preAllocateObjectThreadBased(int thread, int id, size_t size, size_t refCount, int classID);

	allocatorEnum _allocator;
	collectorEnum _collector;
	traversalEnum _traversal;
	writebarriersEnum _writebarrier;

	bool classTableLoaded;
	map< int, string > classTable;

	Allocator* myAllocators[GENERATIONS];
	ObjectContainer* myObjectContainers[GENERATIONS];
	Collector* myGarbageCollectors[GENERATIONS];
	WriteBarrier* myWriteBarrier;

	size_t stats[GENERATIONS];
	Object *parent,*child,*oldChild;
};

}
#endif /* MEMORYMANAGER_HPP_ */
