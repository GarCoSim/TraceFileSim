/*
 * Writebarrier.cpp
 *
 *  Created on: 2016-04-11
 *      Author: Johannes
 */

#include "Writebarrier.hpp"

using namespace std;

namespace traceFileSimulator {


Writebarrier::Writebarrier() {
}

void Writebarrier::setEnvironment(Allocator* allocator) {
	myAllocator = allocator;
}


void Writebarrier::process(Object *parent, Object *oldChild, Object *child) {
	
}

Writebarrier::~Writebarrier() {
}

}
