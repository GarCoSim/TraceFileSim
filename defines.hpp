/*
 * defines.hpp
 *
 *  Created on: 2013-09-04
 *      Author: GarCoSim
 */

#ifndef _DEFINES_HPP_
#define _DEFINES_HPP_

#define VERSION "3.0.1"

#define NUM_THREADS		50
#define ROOTSET_SIZE    50
#define VISUALIZE_GCS 	1
#define OBJECT_HEADER_SIZE 16

#define MAGNITUDE_CONVERSION 1024

//DEBUGGING
#define DEBUG_MODE            	  0
#define WRITE_DETAILED_LOG    	  0
#define WRITE_HEAPMAP         	  0
#define WRITE_ALLOCATION_INFO 	  0
#define FINAL_GC   0 //Trigger a GC after the last line in the trace file

//ERROR HANDLING
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define ERRMSG(...) { \
  fprintf(stderr, "ERROR: %s:%d %s - ", __FILENAME__, __LINE__, __FUNCTION__);\
  fprintf(stderr, __VA_ARGS__);\
  fprintf(stderr, "\n");\
  fflush(stderr);\
}

//GENERATIONAL GC
#define GENERATIONS        1
#define GEN_DEBUG          0
#define GENRATIO           0.3
#define PROMOTIONAGE       3
#define PROMOTIONAGEFACTOR 0
#define SHIFTING           1
#define SHIFTINGFACTOR     2
 
//RECYCLER
#define BLACK 1
#define GREY 2
#define PURPLE 3
#define WHITE 4

#define ZOMBIE 0


enum traversalEnum {
					breadthFirst = 0,
					depthFirst
				};

enum collectorEnum {
						markSweepGC = 0,
						traversalGC,
						recyclerGC
				};

enum allocatorEnum {
						realAlloc = 0,
						basicAlloc,
						nextFitAlloc
				};

enum gcReason {
					reasonStatistics = 0,
					reasonFailedAlloc,
					reasonHighWatermark,
					reasonDebug,
					reasonShift,
					reasonEval,
					reasonForced
				};

enum writebarriersEnum {
					recycler = 0,
					referenceCounting,
					disabled
				};

// create some fancy strings for debug output
#define TRAVERSAL_STRING (traversal == (int)breadthFirst ? "breadthFirst" : "depthFirst")
#define COLLECTOR_STRING (collector == (int)traversalGC ? "traversal" : (collector == (int)markSweepGC ? "markSweep" : (collector == (int)recyclerGC ? "recycler" : "copying")))
#define ALLOCATOR_STRING (allocator == (int)realAlloc ? "real" : (allocator == (int)basicAlloc ? "basic" : "nextFit"))
#define WRITEBARRIER_STRING (writebarrier == (int)recycler ? "recycler" : (writebarrier == (int)referenceCounting ? "referenceCounting" : "disabled"))
#define FINALGC_STRING (finalGC == 1 ? "enabled" : "disabled")

#define CREATE_GLOBAL_FILENAME(name) (globalFilename = (name).substr(0, (name).find(".trace")))

//added by Tristan
#define MAX64BIT 0xFFFFFFFFFFFFFFFF //18446744073709551615 
#define MAX32BIT 0xFFFFFFFF         //4294967295           
#define MAX16BIT 0xFFFF             //65535   				
#define MAX8BIT  0xFF               //255   

#endif
