/*
 * WriteBarrier.cpp
 *
 *  Created on: 2016-04-11
 *      Author: Johannes
 */

#include "WriteBarrier.hpp"

using namespace std;

namespace traceFileSimulator {


WriteBarrier::WriteBarrier() {

}

void WriteBarrier::setEnvironment(Collector* collector) {
	myCollector = collector;
}


void WriteBarrier::process(Object *oldChild, Object *child) {
	
}

WriteBarrier::~WriteBarrier() {
}

}
