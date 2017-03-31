/*
 * main.cpp
 *
 *  Created on: Sep 3, 2013
 *      Author: GarCoSim
 */

#include "Main/Simulator.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <ctime>
#include <string.h>
#include "defines.hpp"

using namespace traceFileSimulator;
using namespace std;
//logging vars
TRACE_FILE_LINE_SIZE gLineInTrace;
int gAllocations;
FILE* gLogFile;
FILE* gDetLog;
FILE* balancedLogFile;
int forceAGCAfterEveryStep = 0;
string globalFilename;
int hierDepth;

FILE* zombieFile;
FILE* lockingFile;
FILE* traversalDepthFile;

int catchZombies;
int countTraversalDepth;
int lockingStats;
int lockNumber;

size_t setHeapSize(int argc, char *argv[], const char *option, const char *shortOption) {
	int i;

	for (i = 1; i < argc; i++) {
		if ((!strcmp("--force", option) || !strcmp("-f", shortOption)) && (!strcmp(argv[i], option) || !strcmp(argv[i], shortOption))) {
			return 1;
		}
		if (!strcmp(argv[i], option) || !strcmp(argv[i], shortOption)) {
			if (!strcmp(option, "--heapsize") || !strcmp(shortOption, "-h")) {

				char suffix;
				char *arg = argv[i + 1];

				// if we have no suffix we can skip this check
				if (isdigit(arg[strlen(arg) - 1]))
					return (size_t)strtoul (arg, NULL, 0);

				suffix = arg[strlen(arg) - 1];

				// get rid of a trailing b/B
				if (suffix == 'B' || suffix == 'b')
					suffix = arg[strlen(arg) - 2];

				switch(suffix) {
					case 'K':
					case 'k':
						return (size_t)strtoul (arg, NULL, 0) * MAGNITUDE_CONVERSION;
					case 'M':
					case 'm':
						return (size_t)strtoul (arg, NULL, 0) * MAGNITUDE_CONVERSION * MAGNITUDE_CONVERSION;
					case 'G':
					case 'g':
						return (size_t)strtoul (arg, NULL, 0) * MAGNITUDE_CONVERSION * MAGNITUDE_CONVERSION * MAGNITUDE_CONVERSION;
					default:
						return (size_t)strtoul (arg, NULL, 0);
				}
			}
		}
		if (!strcmp(argv[i], option) || !strcmp(argv[i], shortOption)) {
			if (!strcmp(option, "--maxheapsize") || !strcmp(shortOption, "-m")) {

				char suffix;
				char *arg = argv[i + 1];

				// if we have no suffix we can skip this check
				if (isdigit(arg[strlen(arg) - 1]))
					return (size_t)strtoul (arg, NULL, 0);

				suffix = arg[strlen(arg) - 1];

				// get rid of a trailing b/B
				if (suffix == 'B' || suffix == 'b')
					suffix = arg[strlen(arg) - 2];

				switch(suffix) {
					case 'K':
					case 'k':
						return (size_t)strtoul (arg, NULL, 0) * MAGNITUDE_CONVERSION;
					case 'M':
					case 'm':
						return (size_t)strtoul (arg, NULL, 0) * MAGNITUDE_CONVERSION * MAGNITUDE_CONVERSION;
					case 'G':
					case 'g':
						return (size_t)strtoul (arg, NULL, 0) * MAGNITUDE_CONVERSION * MAGNITUDE_CONVERSION * MAGNITUDE_CONVERSION;
					default:
						return (size_t)strtoul (arg, NULL, 0);
				}
			}
		}
	}

	return 0;
}

string setLogDirectory(int argc, char *argv[], const char *option, const char *shortOption) {
	int i;

	for (i = 1; i < argc; i++){
		if (!strcmp(argv[i], option) || !strcmp(argv[i], shortOption)) {
			if (!strcmp(option, "--directory") || !strcmp(shortOption, "-d")) {
				string arg = (string)argv[i + 1];
				return arg;
			}
		}
	}
	return "";
}

string setLogLocation(int argc, char *argv[], const char *option, const char *shortOption) {
	int i;

	for (i = 1; i < argc; i++){
		if (!strcmp(argv[i], option) || !strcmp(argv[i], shortOption)) {
			if (!strcmp(option, "--logLocation") || !strcmp(shortOption, "-l")) {
				string arg = (string)argv[i + 1];
				bool containsIllegalChar  = string::npos != arg.find('/')
											|| string::npos != arg.find('\\');
				if(containsIllegalChar){
					const char* errorMsg =
							"File name is a path, use the -d /path/to/directory option to specify a directory for log files.";
					fprintf(stderr, errorMsg);
					throw;
				}
				return arg;
			}
		}
	}
	return "";
}

string getGlobalFileName(string filename){
	string name = filename;
	size_t pos = (string::npos != name.find("/")) ? name.find("/") : name.find('\\');
	while(string::npos != pos) {
		name = name.substr(pos+1);
		pos = (string::npos != name.find("/")) ? name.find("/") : name.find('\\');
	}
	return name.substr(0, name.find(".trace"));
}

string getDefaultLogDirectory(string filename){
	string name = filename;
	size_t pos = (string::npos != name.find("/")) ? name.find("/") : name.find('\\');
	if(string::npos == pos){
		return "./";
	}
	size_t end = 0;
	while(string::npos != pos){
		end += pos+1;
		name = name.substr(pos+1);
		pos = (string::npos != name.find("/")) ? name.find("/") : name.find('\\');
	}
	return filename.substr(0, end);
}

int setArgs(int argc, char *argv[], const char *option, const char *shortOption) {
	int i;
	for (i = 1; i < argc; i++) {
		if ((!strcmp("--force", option) || !strcmp("-f", shortOption)) && (!strcmp(argv[i], option) || !strcmp(argv[i], shortOption))) {
			return 1;
		}
		if (!strcmp(argv[i], option) || !strcmp(argv[i], shortOption)) {
			if (!strcmp(option, "--collector") || !strcmp(shortOption, "-c")) {
				if (!strcmp(argv[i + 1], "markSweep"))
					return (int)markSweepGC;
				if (!strcmp(argv[i + 1], "traversal"))
					return (int)traversalGC;
				if (!strcmp(argv[i + 1], "recycler"))
					return (int)recyclerGC;
				if (!strcmp(argv[i + 1], "balanced"))
					return (int)balanced;
				if (!strcmp(argv[i + 1], "markSweepTB"))
					return (int)markSweepTB;
				return -1;
			} else if (!strcmp(option, "--traversal") || !strcmp(shortOption, "-t")) {
				if (!strcmp(argv[i + 1], "breadthFirst"))
					return (int)breadthFirst;
				if (!strcmp(argv[i + 1], "depthFirst"))
					return (int)depthFirst;
				return -1;
			} else if (!strcmp(option, "--allocator") || !strcmp(shortOption, "-a")) {
				if (!strcmp(argv[i + 1], "basic"))
					return (int)basicAlloc;
				if (!strcmp(argv[i + 1], "nextFit"))
					return (int)nextFitAlloc;
				if (!strcmp(argv[i + 1], "regionBased"))
					return (int)regionBased;
				if (!strcmp(argv[i + 1], "threadBased"))
					return (int)threadBased;
				return -1;
			} else if (!strcmp(option, "--writebarrier") || !strcmp(shortOption, "-wb")) {
				if (!strcmp(argv[i + 1], "disabled"))
					return (int)disabled;
				if (!strcmp(argv[i + 1], "recycler"))
					return (int)recycler;
				if (!strcmp(argv[i + 1], "referenceCounting"))
					return (int)referenceCounting;
				return -1;
			} else if (!strcmp(option, "--finalGC") || !strcmp(shortOption, "-fGC")) {
				if (!strcmp(argv[i + 1], "disabled"))
					return 0;
				if (!strcmp(argv[i + 1], "enabled"))
					return 1;
				return -1;
			} else if (!strcmp(option, "--catchZombies") || !strcmp(shortOption, "-cZ")) {
				if (!strcmp(argv[i + 1], "disabled"))
					return 0;
				if (!strcmp(argv[i + 1], "enabled"))
					return 1;
				return -1;
			} else if (!strcmp(option, "--countTraversalDepth") || !strcmp(shortOption, "-ctd")) {
				if (!strcmp(argv[i + 1], "disabled"))
					return 0;
				if (!strcmp(argv[i + 1], "enabled"))
					return 1;
				return -1;
			} else if (!strcmp(option, "--printLockingStats") || !strcmp(shortOption, "-pls")) {
				if (!strcmp(argv[i + 1], "disabled"))
					return 0;
				if (!strcmp(argv[i + 1], "enabled"))
					return 1;
				return -1;
			} 
		}
	}

	return -1;
}

long long setIdentifier(int argc, char *argv[]) {
	for (int i = 1; i < argc; i++) {
		if(!strcmp(argv[i], "--logIdentifier") || !strcmp(argv[i], "-li")){
			return atoll(argv[i+1]);
		}
	}
	return -1;
}

int main(int argc, char *argv[]) {

	fprintf(stderr, "TraceFileSimulator Version: %s\n\n", VERSION);

	if(argc < 2) {
		fprintf(stderr, "Usage: TraceFileSimulator traceFile [OPTIONS]\n" \
						"Options:\n" \
						"  --watermark x, -w x		uses x percent as the high watermark (default: 90)\n" \
						"  --heapsize x,  -h x		uses x bytes for the heap size (default: Traversal-600000, markSweep-350000)\n" \
						"  --maxheapsize x, -m x    uses x bytes for the maximum heap size (default: heapsize)\n"\
						"  --collector x, -c x		uses x as the garbage collector (valid: markSweep, traversal, recycler, balanced, default: traversal)\n" \
						"  --traversal x, -t x		uses x as the traversal algorithm (valid: breadthFirst depthFirst, default: breadthFirst)\n" \
						"  --allocator x, -a x		uses x as the allocator (valid: real, basic, nextFit, regionBased, default: nextFit)\n" \
						"  --writebarrier x, -wb x	uses x as the write Barrier (valid: referenceCounting, recycler, disabled, default: disabled)\n" \
						"  --finalGC x, -fGC x		uses x as the final GC (valid: disabled, enabled, default: disabled)\n" \
						"  --logLocation x, -l x	uses x as the filename to print the log file (default: trace file's location and name)" \
						"  --directory x, -d x	    uses x as the location to print the log file (default: trace file's location and name)"
						);
		exit(1);
	}

#if WRITE_DETAILED_LOG
		gDetLog = fopen("detailed.log","w+");
#endif

	char *filename		= argv[1];
	size_t heapSize		= setHeapSize(argc, argv, "--heapsize",  "-h");
	size_t maxHeapSize = setHeapSize(argc, argv, "--maxheapsize", "-m");
	int highWatermark	= setArgs(argc, argv, "--watermark", "-w");
	int traversal		= setArgs(argc, argv, "--traversal", "-t");
	int collector		= setArgs(argc, argv, "--collector", "-c");
	int allocator		= setArgs(argc, argv, "--allocator", "-a");
	int writebarrier	= setArgs(argc, argv, "--writebarrier", "-wb");
	int finalGC			= setArgs(argc, argv, "--finalGC", "-fGC");
	string logDirectory = setLogDirectory(argc, argv, "--directory", "-d");
	string customLog	= setLogLocation(argc, argv, "--logLocation", "-l");
	forceAGCAfterEveryStep = setArgs(argc, argv, "--force", "-f");
	catchZombies			= setArgs(argc, argv, "--catchZombies", "-cZ");
	countTraversalDepth 	= setArgs(argc, argv, "--countTraversalDepth", "-ctd");
	lockingStats 			= setArgs(argc, argv, "--printLockingStats", "-pls");

	long long logIdentifier = setIdentifier(argc, argv);


	if (highWatermark == -1)
		highWatermark = 90;
	if (traversal == -1)
		traversal = (int)breadthFirst;
	if (collector == -1)
		collector = (int)traversalGC;
	if (allocator == -1)
		allocator = (int)nextFitAlloc;
	if (writebarrier == -1)
		writebarrier = (int)disabled;
	if (finalGC == -1)
		finalGC = FINAL_GC;
	if (catchZombies == -1)
		catchZombies = 0;
	if (countTraversalDepth == -1)
		countTraversalDepth = 0;
	if (lockingStats == -1)
		lockingStats = 0;
	if (hierDepth == -1){
		hierDepth = HIER_DEPTH_DEFAULT;
	}
	if (heapSize == 0) {
		if (collector != (int)traversalGC)
			heapSize = 350000;
		else
			heapSize = 600000;
	}
	if (maxHeapSize == 0 || maxHeapSize < heapSize)
		maxHeapSize = heapSize;
	if (forceAGCAfterEveryStep == -1)
		forceAGCAfterEveryStep = 0;

	globalFilename = getGlobalFileName(filename);

	string logFileName;
	string balancedLogFileName;
	if(customLog != ""){
		string theWordLog="log";
		if(customLog.substr(customLog.length()-3, 3) == theWordLog){
			logFileName = customLog;
		}
		else{
			logFileName = customLog + ".log";
		}
	}
	else if (forceAGCAfterEveryStep){
		logFileName = globalFilename + "Forced.log";
	}
	else{
		logFileName = globalFilename + ".log";
	}
	balancedLogFileName = "Balanced" + logFileName;

	if(logDirectory == ""){
		logDirectory = getDefaultLogDirectory(filename);
	}

/*	if(escapeAnalysis == -1){
		escapeAnalysis = 0;
	}

	if(clsInfo == -1){
		clsInfo = 0;
	}
*/
	//set up global logfile
	gLogFile = fopen((logDirectory + logFileName).c_str(), "w+");
	if(NULL == gLogFile){
		const char* errorMsg = "Log File failed to open\n";
		fprintf(stderr, errorMsg);
	}
	if(logIdentifier!=-1){
		fprintf(gLogFile, "ID: %lli\n", logIdentifier);
	}

	balancedLogFile = fopen((logDirectory + balancedLogFileName).c_str(), "w+");
	if(NULL == balancedLogFile){
		const char* errorMsg = "Balanced Log File failed to open\n";
		fprintf(stderr, errorMsg);
	}
	fprintf(gLogFile, "TraceFileSimulator Version: %s\nCollector: %s\nTraversal: %s\nAllocator: %s\nHeapsize: %zu%s\nMaximumHeapsize: %zu\nWriteBarrier: %s\nFinal GC: %s\nCatchZombies: %s\nLockingStats: %s\nCountTraversalDepth: %s\nWatermark: %d\n\n",
			VERSION, COLLECTOR_STRING, TRAVERSAL_STRING, ALLOCATOR_STRING, heapSize, collector == traversalGC ? " (split heap)" : "", maxHeapSize, WRITEBARRIER_STRING, FINALGC_STRING, ZOMBIES_STRING, LOCKING_STRING, TRAVERSALDEPTH_STRING, highWatermark);

	fprintf(gLogFile, "%8s | %14s | %10s | %14s "
			"| %13s | %10s | %10s | %10s | %7s | %15s | %12s | %21s\n",
			"Line", "GC Reason", "Total GCs", "Objects Freed",
			"Live Objects", "Heap Used", "Free Heap", "Generation", "GC Time", "Objs freed during GC", "Objs Copied", "Objs Copied during GC");

	fprintf(stderr, "Using tracefile '%s' with a heap size of %zu bytes and a high watermark of %d\n", filename, heapSize, highWatermark);
	fprintf(stderr, "The collector is '%s' and the selected traversal is '%s'\n", COLLECTOR_STRING, TRAVERSAL_STRING);
	fprintf(stderr, "The allocator is '%s'\n", ALLOCATOR_STRING);
	fprintf(stderr, "The writebarrier is '%s'\n", WRITEBARRIER_STRING);
	fprintf(stderr, "The finalGC is '%s'\n", FINALGC_STRING);
	fprintf(stderr, "CatchZombies is '%s'\n", ZOMBIES_STRING);
	fprintf(stderr, "LockingStats is '%s'\n", LOCKING_STRING);
	fprintf(stderr, "CountTraversalDepth is '%s'\n", TRAVERSALDEPTH_STRING);
	if (forceAGCAfterEveryStep)
		fprintf(stderr, "Forcing a GC after every step\n");


	if (countTraversalDepth) {
		string depthFileName = globalFilename + "TraversalDepth.log";
		traversalDepthFile = fopen(depthFileName.c_str(), "w+");
	}

	if (lockingStats) {
		string lockingFileName;
		lockingFileName = globalFilename + "locking.log";
		lockingFile = fopen(lockingFileName.c_str(), "w+");
	}

	if (catchZombies) {
		string zombieFileName;
		zombieFileName = globalFilename + "Zombies.log";
		zombieFile = fopen(zombieFileName.c_str(), "w+");
	}

	//start measuring time
	clock_t start = clock();

	Simulator* simulator = new Simulator(filename, heapSize, maxHeapSize, highWatermark, collector, traversal, allocator, writebarrier, finalGC);

	while(simulator->lastStepWorked() == 1) {
        try {
            simulator->doNextStep();
#if WRITE_DETAILED_LOG
            fflush(gDetLog);
#endif
        } catch(...){
            simulator->lastStats();
            clock_t end = clock();
            double elapsed_secs = double(end - start)/CLOCKS_PER_SEC;
            //double elapsed_msecs = (double)(double)(end - start)/(CLOCKS_PER_SEC/1000);
            printf("Simulation ended with a thrown exception, processed " TRACE_FILE_LINE_FORMAT " lines and execution took %0.3f seconds\n", gLineInTrace, elapsed_secs);
            fprintf(gLogFile,"Execution finished unsuccessfully after %0.3f seconds\n", elapsed_secs);
            if(gLogFile) fclose(gLogFile);
            if(balancedLogFile) fclose(balancedLogFile);
            delete(simulator);
            throw;
        }
	}

	simulator->printStats();
	simulator->lastStats();

	if (lockingStats) {
		if (lockNumber == 0) {
			simulator->updateUnlockedLines();
		}
		else {
			fprintf(stderr, "Locking not zero at end of execution!\n");
		}

		fprintf(lockingFile,"UnLockedLines: %i\n", simulator->getUnlockedLines());
		fprintf(lockingFile,"LockedLines: %i\n", simulator->getLockedLines());
		std::vector<int> lockingCounter = simulator->getLockingCounter();
		for (unsigned int p = 0 ; p < lockingCounter.size(); p++) {
			fprintf(lockingFile,"%i %i\n", p, lockingCounter[p]);
		}
	}

	clock_t end = clock();
	double elapsed_secs = double(end - start)/CLOCKS_PER_SEC;
	//double elapsed_msecs = (double)(double)(end - start)/(CLOCKS_PER_SEC/1000);

	printf("Simulation ended successfully, processed " TRACE_FILE_LINE_FORMAT " lines and execution took %0.3f seconds\n", gLineInTrace, elapsed_secs);
	fprintf(gLogFile,"Execution finished successfully after %0.3f seconds\n", elapsed_secs);

	if (countTraversalDepth) 
		fclose(traversalDepthFile);

	if (lockingStats)
		fclose(lockingFile);

	if (catchZombies)
		fclose(zombieFile);

	fclose(gLogFile);

	fclose(balancedLogFile);

	fclose(traversalDepthFile);

	delete(simulator);

	return EXIT_SUCCESS;
}
