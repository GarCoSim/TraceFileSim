/*
 * Allocator.cpp
 *
 *  Created on: 2013-09-03
 *      Author: GarCoSim
 */

#include "Allocator.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <climits>
#include "../defines.hpp"
#include <string>

extern int gLineInTrace;

using namespace std;

namespace traceFileSimulator {

void Allocator::initializeHeap(int heapSize) {
}

Allocator::Allocator() {
}

void Allocator::setHalfHeapSize(bool value) {
}

void Allocator::moveObject(Object *object) {
}

bool Allocator::isInNewSpace(Object *object) {
	return false;
}

void Allocator::swapHeaps() {
}

void Allocator::freeAllSectors() {
}

size_t Allocator::gcAllocate(int size) {
	return -1;
}

void Allocator::gcFree(Object* object) {
}

int Allocator::getFreeSize() {
	return -1;
}

int Allocator::getHeapSize() {
	return -1;
}

void Allocator::printMap() {
}

void Allocator::printStats() {
}

void Allocator::setAllocationSeearchStart(int address) {
}

Allocator::~Allocator() {
}

}
