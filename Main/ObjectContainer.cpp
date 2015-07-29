/*
 * ObjectContainer.cpp
 *
 *  Created on: 2013-09-03
 *      Author: GarCoSim
 */

#include "ObjectContainer.hpp"

extern int gLineInTrace;

namespace traceFileSimulator {

ObjectContainer::ObjectContainer() {
	int i;
	rootset.resize(NUM_THREADS);
	for (i = 0; i < NUM_THREADS; i++) {
		rootset.at(i).clear();
	}

	objectList.resize(1);
	remSet.resize(1);
	rootCount = 0;
	remCount = 0;
}

// we forward the object in all of our lists
void ObjectContainer::forwardObject(int slot) {
	unsigned int i;
	int id;

	id = objectList.at(slot)->getID();

	// update object list
	objectList.at(slot) = (Object*)objectList.at(slot)->getAddress();
	objectList.at(slot)->updateAddress((size_t)0); // remove forwarding pointer

	// update rootset
	for (i = 0; i < rootset.size(); i++)
		if (isAlreadyRoot(i, id))
			rootset[i][id] = objectList.at(slot);

	// update remset
	for (i = 0; i < remSet.size(); i++)
		if (remSet[i])
			if (remSet[i]->getID() == id)
			remSet[i] = objectList.at(slot);

}

void ObjectContainer::addToGenRoot(Object* object) {
	int remSetSlot = getRemSetSlot();
	if (remSetSlot == -1) {
		fprintf(stderr, "ERROR(Line %d): no remSet slot found\n", gLineInTrace);
		exit(1);
	}
	remSet.at(remSetSlot) = object;
	remCount++;
}

int ObjectContainer::removeFromGenRoot(Object* object){
	int i;
	for(i = 0 ; (unsigned int) i<remSet.size() ; i++){
		if(remSet.at(i) == object){
			remSet.at(i) = NULL;
			remCount--;
			return 0;
		}
	}
	return -1;
}

int ObjectContainer::add(Object* newObject) {
	int id = newObject->getID();
	if ((unsigned int)id >= objectList.size())
		objectList.resize(objectList.size()*2);
	objectList[id] = newObject;
	return 0;
}

int ObjectContainer::removeFromRoot(int thread, int objectID){
	rootset[thread].erase(objectID);
	rootCount--;
	return -1;
}

bool ObjectContainer::doesObjectExistInList(Object *queryObject) {
	int id = queryObject->getID();
	return id<(int)objectList.size() && objectList[id]!=NULL;
}

bool ObjectContainer::isAlreadyRoot(int thread, int objectID) {
	return rootset[thread].find(objectID) != rootset[thread].end();
}

int ObjectContainer::addToRoot(Object* newObject, int thread) {
	if (!doesObjectExistInList(newObject)) {
		add(newObject);
	}
	rootset[thread][newObject->getID()] = newObject;
	rootCount++;

	return 0;
}

vector<Object*> ObjectContainer::getRoots(int thread) {
	vector<Object*> roots;

	std::map<int, Object*>::iterator it;
	for (it=rootset[thread].begin(); it!=rootset[thread].end(); it++)
		roots.push_back(it->second);

	return roots;
}

Object* ObjectContainer::getByID(int id) {
	Object *toReturn = objectList[id];
	if (!toReturn)
		fprintf(stderr, "ERROR(Line %d): object with this id (%d) was not found\n", gLineInTrace, id);
	return toReturn;
}

Object* ObjectContainer::getbySlotNr(int slot) {
	return objectList.at(slot);
}

Object* ObjectContainer::getRoot(int thread, int objectID) {
	return rootset[thread][objectID];
}

int ObjectContainer::deleteObject(Object* object, bool deleteFlag) {
	if (!object)
		fprintf(stderr, "ERROR(Line %d): trying to delete a non existing object\n", gLineInTrace);
	int id = object->getID();
	return deleteObject(id, deleteFlag);
}

int ObjectContainer::deleteObject(int objectID, bool deleteFlag) {
	Object *object = objectList[objectID];
	if (!object) {
		fprintf(stderr, "ERROR(Line %d): object to delete not found. id: %d\n", gLineInTrace, objectID);
		return -1;
	}

	if (object->isForwarded())
		forwardObject(objectID);
	else
		objectList[objectID] = NULL;
	if (deleteFlag)
		delete (object);
	return 0;
}

int ObjectContainer::getSize() {
	return objectList.size();
}

int ObjectContainer::removeReferenceTo(Object* object) {
	int id = object->getID();
	if (!objectList[id])
		return -1;

	objectList[id] = NULL;
	return 0;
}

int ObjectContainer::getGenRootCount() {
	return remCount;
}
int ObjectContainer::getGenRootSize() {
	return remSet.size();
}
int ObjectContainer::getRootSize() {
	return rootCount;
}

Object* ObjectContainer::getGenRoot(int slot) {
	if(slot < 0 && (unsigned int)slot > remSet.size()){
		fprintf(stderr,"ERROR(Line%d):Request illegal gen root. slot:%d\n",
				gLineInTrace,slot);
		exit(1);
	}
	return remSet.at(slot);
}



int ObjectContainer::getRemSetSlot() {
	unsigned int i;
	for (i = 0; i < remSet.size(); i++) {
		if (!remSet.at(i)) {
			return i;
		}

		if (i + 1 == remSet.size()) {
			remSet.resize(remSet.size() * 2);
		}
	}
	fprintf(stderr, "remSetSlotERror: no list slot found\n");
	return -1;
}

int ObjectContainer::getListSlot() {
	fprintf(stderr, "ERROR: ObjectContainer::getListSlot() should never be called.\n");
	return -1;
}
int ObjectContainer::getRootsetSize(int thread){
	return rootset.at(thread).size();
}

void ObjectContainer::clearRemSet(){
	remSet.clear();
	remSet.resize(1);
	remCount = 0;
}

int ObjectContainer::countElements() {
	int result = 0;
	unsigned int i;
	for (i = 0; i < objectList.size(); i++) {
		if (objectList[i]) {
			result++;
		}
	}
	return result;
}

void ObjectContainer::dumpHeap() {
	Object *obj;
	unsigned int i;

	printf("Dumping Heap:\n");
	for (i=0; i<objectList.size(); i++) {
		obj = objectList[i];
		if (obj)
			printf("[object <id:%d>]\n", obj->getID());
	}
	printf("End of Dump\n");
}

ObjectContainer::~ObjectContainer() {
}

} 
