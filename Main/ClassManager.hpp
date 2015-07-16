/*
 * ClassManager.hpp
 *
 *  Created on: 2015-07-15
 *      Author: GarCoSim
 */

#ifndef CLASSMANAGER_HPP_
#define CLASSMANAGER_HPP_

#include <string>
#include <vector>
#include <fstream>

using namespace std;

namespace traceFileSimulator {

	class ClassManager {
		public:
			ClassManager();
			bool isLoaded();
			char *getClassName(int classID);

		private:
			vector<string> classTable;
			bool loaded;
	};

}

#endif /* CLASSMANAGER_HPP_ */

