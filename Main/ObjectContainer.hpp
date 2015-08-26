/*
 * ObjectContainer.hpp
 *
 *  Created on: 2013-09-03
 *      Author: GarCoSim
 */

#ifndef OBJECTCONTAINER_HPP_
#define OBJECTCONTAINER_HPP_

#include <tr1/unordered_map>
#include <vector>
#include "Object.hpp"
#include <stdio.h>
#include <stdlib.h>
#include "../defines.hpp"
#include <algorithm>
 
namespace traceFileSimulator {

class ObjectContainer {
public:
	ObjectContainer();
	virtual ~ObjectContainer();
	int add(Object* newObject);
	int addToRoot(Object* newObject, int thread);
	int removeFromRoot(int thread, int objectID);
	void addToGenRoot(Object* object);
	int removeFromGenRoot(Object* object);
	int removeReferenceTo(Object* object);
	Object* getByID(int id);
	Object* getRoot(int thread, int objectID);
	int getSize();
	int deleteObject(Object* object, bool deleteFlag);
	int deleteObject(int objectID, bool deleteFlag);
	int getGenRootSize();
	int getGenRootCount();
	int getRootSize();
	Object* getGenRoot(int slot);
	void clearRemSet();
	vector<Object*> getRoots(int thread);
	vector<Object*> getLiveObjects();
	vector<Object*> getLiveObjectsInHeapOrder();
	int getRootsetSize(int thread);
//	void visualizeState(char* filename);
	int countElements();
	bool isAlreadyRoot(int thread, int objectID);
	void dumpHeap();
private:
	bool doesObjectExistInList(Object *queryObject);
	int getRemSetSlot();

	vector<std::tr1::unordered_map<int, Object*> > rootset;
	std::tr1::unordered_map<int, Object*> objectMap;
	vector<Object*> remSet;

	int rootCount;
	int remCount;
};

}
#endif /* OBJECTCONTAINER_HPP_ */
