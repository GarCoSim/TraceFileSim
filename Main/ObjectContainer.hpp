/*
 * ObjectContainer.hpp
 *
 *  Created on: 2013-09-03
 *      Author: GarCoSim
 */

#ifndef OBJECTCONTAINER_HPP_
#define OBJECTCONTAINER_HPP_

#include <vector>
#include "Object.hpp"
#include <stdio.h>
#include <stdlib.h>
#include "../defines.hpp"
 
namespace traceFileSimulator {

class ObjectContainer {
public:
	ObjectContainer();
	virtual ~ObjectContainer();
	int add(Object* newObject);
	int addToRoot(Object* newObject, int thread, int rootSlot);
	int removeFromRoot(int thread, int root );
	void addToGenRoot(Object* object);
	int removeFromGenRoot(Object* object);
	int removeReferenceTo(Object* object);
	Object* getByID(int id);
	Object* getbySlotNr(int slot);
	Object* getRoot(int thread, int rootSlot);
	int getSize();
	int deleteObject(Object* object, bool deleteFlag);
	int deleteObject(int objectID, bool deleteFlag);
	int getGenRootSize();
	int getGenRootCount();
	int getRootSize();
	Object* getGenRoot(int slot);
	void clearRemSet();
	int getRootsetSlot(int thread);
	int getRootsetIndexByID(int thread, int id);
	int getRootsetSize(int thread);
//	void visualizeState(char* filename);
	int countElements();
	bool isAlreadyRoot(int thread, int id);
	void forwardObject(int slot);
private:
	int getListSlot();
	bool doesObjectExistInList(Object *queryObject);
	int getRemSetSlot();

	vector<vector<Object*> > rootset;
	vector<Object*> objectList;
	vector<Object*> remSet;

	int rootCount;
	int remCount;
};

}
#endif /* OBJECTCONTAINER_HPP_ */
