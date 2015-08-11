/*
 * Object.cpp
 *
 *  Created on: Jul 30, 2013
 *      Author: GarCoSim
 */

#include "Object.hpp"

namespace traceFileSimulator {

Object::Object(int id, int payloadSize, int maxPointers, int address, char *className) {
	setArgs(id, payloadSize, maxPointers, className);
	myAddress = address;
}

// this only needs to be run when we create the objects in the real allocator
void Object::setArgs(int id, int payloadSize, int maxPointers, char *className) {

	//prepare data structure
	myId = id;
	myPayloadSize = payloadSize;
	myPointersCurrent = 0;
	myPointersMax = maxPointers;


	pointers = (Object**)malloc(maxPointers*sizeof(Object*));
	for(int i = 0; i < maxPointers;i++){
		pointers[i] = NULL;
	}

	myGeneration = 0;
	myAge = 0;
	myName = className;

	//stat
	isAlive = 0;
	isVisited = 0;
	freed = 0;
	myAddress = 0;
	forwarded = false;
}

// added by Tristan




void Object::setGeneration(int generation){
	myGeneration = generation;
}

int Object::getGeneration(){
	return myGeneration;
}

size_t Object::getAddress(){
	return myAddress;
}

int Object::getID(){
	return this->myId;
}

int Object::getPayloadSize(){
	return myPayloadSize;
}

int Object::getPointerCount(){
	return myPointersCurrent;
}

int Object::getPointersMax(){
	return myPointersMax;
}
Object* Object::getReferenceTo(int pointerNumber){
	return pointers[pointerNumber];
}

int Object::setPointer(int pointerNumber, Object* target){

	if(pointerNumber >= myPointersMax){
		fprintf(stderr, "ERROR in Object: set Pointer to impossible slot\n");
		fflush(stdout);
		return 0;
	}
 
	pointers[pointerNumber] = target;
	return 1;
}

int Object::getIsAlive(){
	return isAlive;
}

void Object::setIsAlive(int value){
	isAlive = value;
}

int Object::getVisited(){
	return isVisited;
}

void Object::updateAddress(size_t newAddress) {
	myAddress = newAddress;
}

void Object::setVisited(int value){
	isVisited = value;
}


//the following were added by Tristan
void Object::setPtrsNull(int numPtrs) {
	int i;

	for (i=0; i<numPtrs; i++)
		pointers[i] = NULL;
}

void Object::resetPtrs(int numPtrs) {
	
	pointers = (Object**)((unsigned long)this+(unsigned long)sizeof(Object)); //point to end of object header
	setPtrsNull(numPtrs); //not clear why we have to do this, my hunch is, null value is checked before updating the pointers later

}

void Object::setArgsReal(int id, int payloadSize, int maxPointers, char *className) {

	//prepare data structure
	myId = id;
	myPayloadSize = payloadSize;
	myPointersCurrent = 0;
	myPointersMax = maxPointers;

	pointers = (Object**)((unsigned long)this+(unsigned long)sizeof(Object)); //point to end of object header
	setPtrsNull(maxPointers);

	myGeneration = 0;
	myAge = 0;
	myName = className;

	//stat
	isAlive = 0;
	isVisited = 0;
	freed = 0;
	myAddress = 0;
	forwarded = false;
}



Object::~Object() {}

}
