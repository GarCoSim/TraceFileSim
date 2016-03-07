/*
 * Object.cpp
 *
 *  Created on: Jul 30, 2013
 *      Author: GarCoSim
 */

#include "../defines.hpp"
#include "Object.hpp"
#include <stdio.h>

extern int gLineInTrace; //added by Tristan

namespace traceFileSimulator {

Object::Object(int id, void *address, size_t size, int maxPointers, char *className) {
	myId = id;
	rawObject = (RawObject *) address;
	rawObject->associatedObject = this;
	mySize = size;
	myPointersMax = maxPointers;
	for (int i=0; i<myPointersMax; i++)
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

Object::Object(int tid, int id, void *address, size_t size, int maxPointers, char *className) {
	myId = id;
	rawObject = (RawObject *) address;
	rawObject->associatedObject = this;
	mySize = size;
	myPointersMax = maxPointers;
	for (int i=0; i<myPointersMax; i++)
		rawObject->pointers[i] = NULL;
	myGeneration = 0;
	myAge = 0;
	myName = className;

	// stats
	isVisited = false;
	freed = 0;
	forwarded = false;
	forwardedPointer = rawObject;

	// added by mazder
	myTid = tid;
	escaped = false;
	//start measuring life time
	struct timeval tv;
	gettimeofday(&tv, NULL);
	born = 1000000*tv.tv_sec+tv.tv_usec;

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

size_t Object::getPayloadSize(){
	return mySize - OBJECT_HEADER_SIZE;
}

size_t Object::getHeapSize(){
	return mySize;
}

int Object::getPointersMax(){
	return myPointersMax;
}

Object* Object::getReferenceTo(int pointerNumber){
	RawObject *target = rawObject->pointers[pointerNumber];
	if (target)
		return target->associatedObject;
	return NULL;
}

int Object::setPointer(int pointerNumber, Object* target){

	if(pointerNumber >= myPointersMax){
		fprintf(stderr, "ERROR (%d) in Object (%d): set Pointer to impossible slot\n",gLineInTrace,target->getID()); //modified by Tristan; added line number and object ID
		fflush(stdout);
		return 0;
	}

	if (target)
		rawObject->pointers[pointerNumber] = target->rawObject;
	else
		rawObject->pointers[pointerNumber] = NULL;
	return 1;
}

void Object::setForwardedPointer (Object* target){
	forwardedPointer = target->rawObject;
}

Object* Object::getForwardedPointer (){
	return forwardedPointer->associatedObject;
}

void *Object::getRawPointerAddress(int pointerNumber) {
	return (void *) rawObject->pointers[pointerNumber];
}

void Object::setRawPointerAddress(int pointerNumber, void *address) {
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

int Object::getReferenceCount() {
	return referenceCount;
}

void Object::increaseReferenceCount() {
	referenceCount++;
}

size_t Object::getRegion(size_t heapStart, size_t regionSize) {
       return (size_t)(((size_t)rawObject-heapStart)/(size_t)regionSize);
}

void Object::decreaseReferenceCount() {
	//if (referenceCount > 0)
		referenceCount--;
	//if (referenceCount == 0)
	//	fprintf(stderr, "RC dropped to zero\n");
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
