/*
 * Object.cpp
 *
 *  Created on: Jul 30, 2013
 *      Author: GarCoSim
 */

#include "Object.hpp"
#include <stdio.h>

extern LINESIZE gLineInTrace;

namespace traceFileSimulator {

/** Class containing metadata about objects
 *
 * @param id ID of allocated object
 * @param address Address of allocated object in the heap
 * @param size Size of allocated object in the heap (including object header size)
 * @param maxPointers Number of references the object has
 * @param className Name of the class of the object
 * @param _allocationType Used to determine child pointer semantics
 */
Object::Object(int id, void *address, size_t size, size_t maxPointers, char *className, allocationTypeEnum _allocationType) {
	myId = id;
	allocationType = _allocationType;
	rawObject = (RawObject *) address;
	rawObject->associatedObject = this;
	mySize = size;
	myPointersMax = maxPointers;
	for (size_t i=0; i<myPointersMax; i++)
		rawObject->pointers[i] = NULL;
	myGeneration = 0;
	myAge = 0;
	myName = className;

	// stats
	isVisited = false;
	freed = 0;
	forwarded = false;

	forwardedPointer = rawObject;

	referenceCount = 0;
	color = BLACK;
}

void Object::setGeneration(int generation){
	myGeneration = generation;
}

int Object::getGeneration(){
	return myGeneration;
}

void *Object::getAddress(){
	return (void *) rawObject;
}

int Object::getID(){
	return this->myId;
}

size_t Object::getHeapSize(){
	return mySize;
}

size_t Object::getPointersMax(){
	return myPointersMax;
}

/** Gets the address in this objects slot
 *
 * @param pointerNumber Slot to get the address of
 * @return
 */
Object* Object::getReferenceTo(size_t pointerNumber){
	RawObject *target = rawObject->pointers[pointerNumber];
	if (target)
	{
		return target->associatedObject;
	}
	return NULL;
}

/** Sets this object's pointer in slot pointerNumber to the value of target
 *
 * @param pointerNumber Slot in this object to set
 * @param target Object to be pointed to
 * @return
 */
int Object::setPointer(size_t pointerNumber, Object* target){
	if(this->allocationType == allocationTypeObject){
		if(pointerNumber >= myPointersMax){
			fprintf(stderr, "ERROR (%lld) in Object (%d): set Pointer to impossible slot\n",gLineInTrace,target->getID());
			fflush(stdout);
			return 0;
		}

		if (target)
			rawObject->pointers[pointerNumber] = target->rawObject;
		else
			rawObject->pointers[pointerNumber] = NULL;
	}
	else if(this->allocationType == allocationTypeDiscontiguousIndexable){
		rawObject->pointers[pointerNumber] = target->rawObject;
	}
	return 1;
}

void Object::setForwardedPointer (void* address){
	forwardedPointer = address;
}

void* Object::getForwardedPointer (){
	return forwardedPointer;
}

void *Object::getRawPointerAddress(size_t pointerNumber) {
	return (void *) rawObject->pointers[pointerNumber];
}

void Object::setRawPointerAddress(size_t pointerNumber, void *address) {
	rawObject->pointers[pointerNumber] = (RawObject *) address;
}

bool Object::getVisited(){
	return isVisited;
}

void Object::updateAddress(void *newAddress) {
	rawObject = (RawObject *) newAddress;
	rawObject->associatedObject = this;
}

void Object::setVisited(bool value){
	isVisited = value;
}

int Object::getDepth(){
	return depth;
}
void Object::setDepth(int d){
	depth = d;
}

size_t Object::getReferenceCount() {
	return referenceCount;
}

void Object::increaseReferenceCount() {
	referenceCount++;
}

void Object::decreaseReferenceCount() {
	referenceCount--;
}

int Object::getColor() {
	return color;
}

void Object::setColor(int color) {
	this->color = color;
}

Object::~Object() {

}

}
