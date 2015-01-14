/*
 * main.cpp
 *
 *  Created on: Sep 3, 2013
 *      Author: kons
 */

#include "Objects/Simulator.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctime>

using namespace traceFileSimulator;
using namespace std;
//logging vars
int gLineInTrace;
int gAllocations;
FILE* gLogFile;
FILE* gDetLog;

int main(int argc, char *argv[]) {
	if(argc != 4){
		fprintf(stderr,"Not enough arguments provided. \n"
				"Usage: GCKons traceFile heapsize highWatermark\n");
		exit(1);
	}

	if(WRITE_DETAILED_LOG == 1){
		gDetLog = fopen("detailed.log","w+");
	}


	//set up global logfile
	gLogFile = fopen("gcLog.log", "w+");
	fprintf(gLogFile, "%8s | %9s | %9s | %14s "
			"| %13s | %10s | %10s |\n",
			"LINE", "GC REASON", "Total GCs:", "Objects freed:", "live objects:",
			"heap used:", "free heap:");

	char* filename = argv[1];
	int heapSize = atoi(argv[2]);
	int highWatermark = atoi(argv[3]);

	//start measuring time
	clock_t start = clock();

	Simulator* simulator = new Simulator(filename, heapSize, highWatermark);

	while(simulator->lastStepWorked() == 1){
		simulator->doNextStep();
		if(WRITE_DETAILED_LOG == 1){
			fflush(gDetLog);
		}

	}
	//
	simulator->printStats();

	clock_t end = clock();
	double elapsed_secs = double(end - start)/CLOCKS_PER_SEC;
	//double elapsed_msecs = (double)(double)(end - start)/(CLOCKS_PER_SEC/1000);
	printf("End. Execution took: %f seconds\n", elapsed_secs);
	fprintf(gLogFile,"Execution finished after %f seconds\n", elapsed_secs);


	fclose(gLogFile);

	return 0;
}


