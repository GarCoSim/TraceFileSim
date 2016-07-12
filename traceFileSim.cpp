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
int gLineInTrace;
int gAllocations;
FILE* gLogFile;
FILE* gDetLog;
FILE* balancedLogFile;
int forceAGCAfterEveryStep = 0;
string globalFilename;
int hierDepth;

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

string setLogLocation(int argc, char *argv[], const char *option, const char *shortOption) {
	int i;

	for (i = 1; i < argc; i++){
		if (!strcmp(argv[i], option) || !strcmp(argv[i], shortOption)) {
			if (!strcmp(option, "--logLocation") || !strcmp(shortOption, "-l")) {
				string arg = (string)argv[i + 1];
				return arg;
			}
		}
	}
	return "";
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
				return -1;
			} else if (!strcmp(option, "--traversal") || !strcmp(shortOption, "-t")) {
				if (!strcmp(argv[i + 1], "breadthFirst"))
					return (int)breadthFirst;
				if (!strcmp(argv[i + 1], "depthFirst"))
					return (int)depthFirst;
				return -1;
			} else if (!strcmp(option, "--allocator") || !strcmp(shortOption, "-a")) {
				if (!strcmp(argv[i + 1], "real"))
					return (int)realAlloc;
				if (!strcmp(argv[i + 1], "basic"))
					return (int)basicAlloc;
				if (!strcmp(argv[i + 1], "nextFit"))
					return (int)nextFitAlloc;
				if (!strcmp(argv[i + 1], "regionBased"))
					return (int)regionBased;
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
						"  --maxheapsize x, -m x     uses x bytes for the maximum heap size (default: heapsize)\n"\
						"  --collector x, -c x		uses x as the garbage collector (valid: markSweep, traversal, recycler, default: traversal)\n" \
						"  --traversal x, -t x		uses x as the traversal algorithm (valid: breadthFirst depthFirst, default: breadthFirst)\n" \
						"  --allocator x, -a x		uses x as the allocator (valid: real, basic, nextFit default: nextFit)\n" \
						"  --writebarrier x, -wb x	uses x as the write Barrier (valid: referenceCounting, recycler, disabled, default: disabled)\n" \
						"  --finalGC x, -fGC x		uses x as the final GC (valid: disabled, enabled, default: disabled)\n" \
						"  --logLocation x, -l x	uses x as the location and filename to print the log file (default: trace file's location and name)"
						);
		exit(1);
	}

	fprintf(stderr, "TraceFileSimulator v%lf\n\n", VERSION);

	if(WRITE_DETAILED_LOG) {
		gDetLog = fopen("detailed.log","w+");
	}

	char *filename		= argv[1];
	size_t heapSize		= setHeapSize(argc, argv, "--heapsize",  "-h");
	size_t maxHeapSize = setHeapSize(argc, argv, "--maxheapsize", "-m");
	int highWatermark	= setArgs(argc, argv, "--watermark", "-w");
	int traversal		= setArgs(argc, argv, "--traversal", "-t");
	int collector		= setArgs(argc, argv, "--collector", "-c");
	int allocator		= setArgs(argc, argv, "--allocator", "-a");
	int writebarrier	= setArgs(argc, argv, "--writebarrier", "-wb");
	int finalGC			= setArgs(argc, argv, "--finalGC", "-fGC");
	string customLog	= setLogLocation(argc, argv, "--logLocation", "-l");
	forceAGCAfterEveryStep = setArgs(argc, argv, "--force", "-f");
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

	CREATE_GLOBAL_FILENAME((string)filename);

	string logFileName;
	char *customLogChar = &customLog[0u];
	if(strcmp(customLogChar, "")){
		string theWordLog="log";
		if(customLog.substr(customLog.length()-3, 3).compare(theWordLog)==0){
			logFileName = customLog;
		}
		else{
			logFileName = customLog + ".log";
		}
	}
	else if (forceAGCAfterEveryStep)
		logFileName = globalFilename + "Forced.log";
	else
		logFileName = globalFilename + ".log";

	if(escapeAnalysis == -1){
		escapeAnalysis = 0;
	}

	if(clsInfo == -1){
		clsInfo = 0;
	}

	string balancedLogFileName;
	balancedLogFileName = globalFilename + "Balanced.log";

	//set up global logfile
	gLogFile = fopen(logFileName.c_str(), "w+");
	if(logIdentifier!=-1){
		fprintf(gLogFile, "ID: %lli\n", logIdentifier);
	}
	balancedLogFile = fopen(balancedLogFileName.c_str(), "w+");

	fprintf(gLogFile, "TraceFileSimulator Version: %s\nCollector: %s\nTraversal: %s\nAllocator: %s\nHeapsize: %zu%s\nMaximumHeapsize: %zuWriteBarrier: %s\nFinal GC: %s\nWatermark: %d\n\n",
			VERSION, COLLECTOR_STRING, TRAVERSAL_STRING, ALLOCATOR_STRING, heapSize, collector == traversalGC ? " (split heap)" : "", maxHeapSize, WRITEBARRIER_STRING, FINALGC_STRING, highWatermark);
	fprintf(gLogFile, "%8s | %14s | %10s | %14s "
			"| %13s | %10s | %10s | %10s | %7s | %15s | %12s | %21s\n",
			"Line", "GC Reason", "Total GCs", "Objects Freed",
			"Live Objects", "Heap Used", "Free Heap", "Generation", "GC Time", "Objs freed during GC", "Objs Copied", "Objs Copied during GC");

	fprintf(stderr, "Using tracefile '%s' with a heap size of %zu bytes and a high watermark of %d\n", filename, heapSize, highWatermark);
	fprintf(stderr, "The collector is '%s' and the selected traversal is '%s'\n", COLLECTOR_STRING, TRAVERSAL_STRING);
	fprintf(stderr, "The allocator is '%s'\n", ALLOCATOR_STRING);
	fprintf(stderr, "The writebarrier is '%s'\n", WRITEBARRIER_STRING);
	fprintf(stderr, "The final GC is '%s'\n", FINALGC_STRING);
	if (forceAGCAfterEveryStep)
		fprintf(stderr, "Forcing a GC after every step\n");

	//start measuring time
	clock_t start = clock();

	Simulator* simulator = new Simulator(filename, heapSize, maxHeapSize, highWatermark, collector, traversal, allocator, writebarrier, finalGC);

	while(simulator->lastStepWorked() == 1) {
		simulator->doNextStep();
		if(WRITE_DETAILED_LOG) {
			fflush(gDetLog);
		}
	}

	simulator->printStats();
	simulator->lastStats();

	clock_t end = clock();
	double elapsed_secs = double(end - start)/CLOCKS_PER_SEC;
	//double elapsed_msecs = (double)(double)(end - start)/(CLOCKS_PER_SEC/1000);
	printf("Simulation ended successfully, execution took %0.3f seconds\n", elapsed_secs);
	fprintf(gLogFile,"Execution finished after %0.3f seconds\n", elapsed_secs);

	fclose(gLogFile);
	fclose(balancedLogFile);

	delete(simulator);

	return EXIT_SUCCESS;
}
