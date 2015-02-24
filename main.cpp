/*
 * main.cpp
 *
 *  Created on: Sep 3, 2013
 *      Author: GarCoSim
 */

#include "Objects/Simulator.hpp"
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

int setArgs(int argc, char *argv[], const char *option, const char *shortOption) {
	int i;

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], option) || !strcmp(argv[i], shortOption)) {
			if (!strcmp(option, "--collector") || !strcmp(shortOption, "-c")) {
				if (!strcmp(argv[i + 1], "copying"))
					return (int)copyingGC;
				if (!strcmp(argv[i + 1], "markSweep"))
					return (int)markSweepGC;
			} else
				return atoi(argv[i + 1]); // be careful! we expect the next one to be a number, otherwise we crash instantly
		}
	}

	return -1;
}

int main(int argc, char *argv[]) {
	if(argc < 2) {
		fprintf(stderr, "Usage: TraceFileSimulator traceFile [OPTIONS]\n" \
						"Options:\n" \
						"  --watermark x, -w x       uses x percent as the high watermark (default: 90)\n" \
						"  --heapsize x,  -h x       uses x bytes for the heap size (default: 200000)\n" \
						"  --collector x, -c x       uses x as the garbage collector (valid: copying, markSweep, default: markSweep)\n"
						);
		exit(1);
	}

	if(WRITE_DETAILED_LOG) {
		gDetLog = fopen("detailed.log","w+");
	}

	//set up global logfile
	gLogFile = fopen("gcLog.log", "w+");
	fprintf(gLogFile, "%8s | %9s | %9s | %14s "
			"| %13s | %10s | %10s |\n",
			"LINE", "GC REASON", "Total GCs:", "Objects freed:", "Live objects:",
			"Heap used:", "Free heap:");

	char* filename    = argv[1];
	int heapSize      = setArgs(argc, argv, "--heapsize",  "-h");
	int highWatermark = setArgs(argc, argv, "--watermark", "-w");
	int collector     = setArgs(argc, argv, "--collector", "-c");

	if (heapSize == -1)
		heapSize = 200000;
	if (highWatermark == -1)
		highWatermark = 90;
	if (collector == -1)
		collector = (int)markSweepGC;

	fprintf(stderr, "Using tracefile '%s' with a heap size of %d bytes and a high watermark of %d\n", filename, heapSize, highWatermark);

	//start measuring time
	clock_t start = clock();

	Simulator* simulator = new Simulator(filename, heapSize, highWatermark, collector);

	while(simulator->lastStepWorked() == 1) {
		simulator->doNextStep();
		if(WRITE_DETAILED_LOG) {
			fflush(gDetLog);
		}
	}
	simulator->printStats();

	clock_t end = clock();
	double elapsed_secs = double(end - start)/CLOCKS_PER_SEC;
	//double elapsed_msecs = (double)(double)(end - start)/(CLOCKS_PER_SEC/1000);
	printf("Simulation ended successfully, execution took %0.3f seconds\n", elapsed_secs);
	fprintf(gLogFile,"Execution finished after %0.3f seconds\n", elapsed_secs);


	fclose(gLogFile);

	return EXIT_SUCCESS;
}


