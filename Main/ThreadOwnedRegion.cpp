/*
 * ThreadOwnedRegion.cpp
 *
 *  Created on: 2015-10-30
 *      Author: GarCoSim
 *
 */

#include "ThreadOwnedRegion.hpp"


namespace traceFileSimulator {

ThreadOwnedRegion::ThreadOwnedRegion(void *address,int size,int owner) : Region(address, size) {
	myOwner = owner;
}



void ThreadOwnedRegion::setOwner(int owner) {
	myOwner = owner;
}

int ThreadOwnedRegion::getOwner() {
	return myOwner;
}


ThreadOwnedRegion::~ThreadOwnedRegion() {
}


}