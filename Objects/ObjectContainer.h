/*
 * ObjectContainer.h
 *
 *  Created on: 2013-09-03
 *      Author: kons
 */

#ifndef OBJECTCONTAINER_H_
#define OBJECTCONTAINER_H_

#include <vector>
#include "Object.h"

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
	int deleteObject(Object* object);
	int deleteObject(int objectID);
	int getGenRootSize();
	int getGenRootCount();
	int getRootSize();
	Object* getGenRoot(int slot);
	void clearRemSet();
//	void visualizeState(char* filename);
	int countElements();
private:
	int getListSlot();
	int getRemSetSlot();
	//two dimensional rootset [threadNum][rootsetSize]
	vector<vector<Object*> > rootset;
	//collection of all objects (dynamic)
	vector<Object*> objectList;
	vector<Object*> remSet;
	int rootCount;
	int remCount;
};

} /* namespace gcKons */
#endif /* OBJECTCONTAINER_H_ */
