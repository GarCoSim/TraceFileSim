/*
 * ClassManager.cpp
 *
 *  Created on: 2015-07-15
 *      Author: GarCoSim
 */

#include "ClassManager.hpp"

using namespace std;

extern string globalFilename;

namespace traceFileSimulator {

	ClassManager::ClassManager() {
		loaded = false;

		ifstream classFile;
		size_t found;
		string line, className = globalFilename + ".cls";

		// we need to push an empty element into the vector as our classes start with id 1
		classTable.push_back("EMPTY");

		classFile.open(className.c_str());
		if (!classFile.good())
			return;

		do {
			if(getline(classFile, line)) {
				found = line.find(": ");
				line = line.substr(found + 2, line.size() - found - 2);
				classTable.push_back(line);
			}
		} while (!classFile.eof());

		loaded = true;
	}

	bool ClassManager::isLoaded() {
		return loaded;
	}

	char *ClassManager::getClassName(int classID) {
		if (!loaded)
			return (char*) "CLASS_TABLE_NOT_LOADED";
		else if (classID > (int)classTable.size())
			return (char*) "OUT_OF_BOUNDS";
		else
			return (char*) classTable.at(classID).c_str();
	}

}
