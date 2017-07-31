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

/** Sets the backup collector for the reference count algorithm.
 *
 * @param collector Collector implementation to be used for collections
 */
void WriteBarrier::setEnvironment(Collector* collector) {
	myCollector = collector;
}

WriteBarrier::~WriteBarrier() {
}

}
