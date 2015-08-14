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

	objectMap.clear();
	remSet.resize(1);
	rootCount = 0;
	remCount = 0;
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
	objectMap[id] = newObject;
	return 0;
}

int ObjectContainer::removeFromRoot(int thread, int objectID){
	rootset[thread].erase(objectID);
	rootCount--;
	return -1;
}

bool ObjectContainer::doesObjectExistInList(Object *queryObject) {
	int id = queryObject->getID();
	return objectMap.find(id) != objectMap.end();
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

	std::tr1::unordered_map<int, Object*>::iterator it;
	for (it=rootset[thread].begin(); it!=rootset[thread].end(); it++)
		roots.push_back(it->second);

	return roots;
}

vector<Object*> ObjectContainer::getLiveObjects() {
	vector<Object*> objects;

	std::tr1::unordered_map<int, Object*>::iterator it;
	for (it=objectMap.begin(); it!=objectMap.end(); it++)
		objects.push_back(it->second);

	return objects;
}

Object* ObjectContainer::getByID(int id) {
	if (objectMap.find(id) == objectMap.end())
		fprintf(stderr, "ERROR(Line %d): object with this id (%d) was not found\n", gLineInTrace, id);
	return objectMap[id];
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
	Object *object = objectMap[objectID];
	if (!object) {
		fprintf(stderr, "ERROR(Line %d): object to delete not found. id: %d\n", gLineInTrace, objectID);
		return -1;
	}

	objectMap.erase(objectID);
	if (deleteFlag)
		delete (object);
	return 0;
}

int ObjectContainer::getSize() {
	return objectMap.size();
}

int ObjectContainer::removeReferenceTo(Object* object) {
	int id = object->getID();
	if (objectMap.find(id) == objectMap.end()) // not found
		return -1;

	objectMap.erase(id);
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

int ObjectContainer::getRootsetSize(int thread){
	return rootset.at(thread).size();
}

void ObjectContainer::clearRemSet(){
	remSet.clear();
	remSet.resize(1);
	remCount = 0;
}

int ObjectContainer::countElements() {
	return objectMap.size();
}

void ObjectContainer::dumpHeap() {
	unsigned int i;
	vector<Object*> objects = getLiveObjects();
	printf("Dumping Heap:\n");
	for(i=0; i<objects.size(); i++)
		printf("<object: id=%d>\n", objects[i]->getID());
	printf("End of Dump\n");
}

ObjectContainer::~ObjectContainer() {
}

} 
