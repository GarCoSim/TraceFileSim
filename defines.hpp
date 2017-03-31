/*
 * defines.hpp
 *
 *  Created on: 2013-09-04
 *      Author: GarCoSim
 */

#ifndef _DEFINES_HPP_
#define _DEFINES_HPP_

#ifndef VERSION
#define VERSION "4.0.0"
#endif

#ifndef NUM_THREADS
#define NUM_THREADS		50
#endif

#ifndef ROOTSET_SIZE
#define ROOTSET_SIZE	50
#endif

#ifndef  VISUALIZE_GCS
#define VISUALIZE_GCS 	1
#endif

#ifndef OBJECT_HEADER_SIZE
#define OBJECT_HEADER_SIZE 16
#endif

#define MAGNITUDE_CONVERSION 1024

//DEBUGGING FLAGS
#ifndef DEBUG_MODE
#define DEBUG_MODE				0
#endif

#ifndef WRITE_DETAILED_LOG
#define WRITE_DETAILED_LOG		0
#endif

#ifndef WRITE_HEAPMAP
#define WRITE_HEAPMAP			0
#endif

#ifndef WRITE_ALLOCATION_INFO
#define WRITE_ALLOCATION_INFO	0
#endif

#ifndef FINAL_GC
#define FINAL_GC				0 //Trigger a GC after the last line in the trace file
#endif
//END DEBUGGING FLAGS

//TRACE FILE LINE DEFINES
#ifndef TRACE_FILE_LINE_SIZE
#define TRACE_FILE_LINE_SIZE unsigned long long
#define TRACE_FILE_LINE_FORMAT "%lld"
#endif
//END TRACE FILE LINE DEFINES

//ERROR HANDLING
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define ERRMSG(...) { \
  fprintf(stderr, "ERROR: %s:%d %s - ", __FILENAME__, __LINE__, __FUNCTION__);\
  fprintf(stderr, __VA_ARGS__);\
  fprintf(stderr, "\n");\
  fflush(stderr);\
}

//GENERATIONAL GC
#define GENERATIONS			1
#define GEN_DEBUG			0
#define GENRATIO			0.3
#define PROMOTIONAGE		3
#define PROMOTIONAGEFACTOR	0
#define SHIFTING			1
#define SHIFTINGFACTOR		2

//RECYCLER
#define BLACK 1
#define GREY 2
#define PURPLE 3
#define WHITE 4

#define ZOMBIE 0

//BALANCED GC
//#define MINREGIONSIZE	512000 //in case of change, also change REGIONEXPONENT appropriately
//#define REGIONEXPONENT	9 //9 results in MINREGINOSIZE of 512KB; 2^9 = 512KB
#define MINREGIONSIZE	2000 //in case of change, also change REGIONEXPONENT appropriately
#define REGIONEXPONENT	1 //2 results in MINREGINOSIZE of 512KB; 2^1 = 2KB
#define MINREGIONS		1024
#define MAXREGIONS		2047
#define EDENREGIONS		25 //% of the whole heap
#define MAXREGIONAGE	23

#define DEAD_SPACE				1 //Consider space occupied by dead objects when selecting collection set regions
#define DEAD_SPACE_THRESHOLD	10 //Percent dead space a region must have to be selected, 0 to select regions with highest percent

enum allocationTypeEnum{
					allocationTypeObject = 0,
					allocationTypeContiguousIndexable,
					allocationTypeDiscontiguousIndexable
				};

enum traversalEnum {
					breadthFirst = 0,
					depthFirst,
					hierarchical
				};

enum collectorEnum {
						markSweepGC = 0,
						traversalGC,
						recyclerGC,
						balanced,
						markSweepTB
				};
#define HIER_DEPTH_DEFAULT 2

enum allocatorEnum {
						basicAlloc = 0,
						nextFitAlloc,
						regionBased,
						threadBased
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
#define COLLECTOR_STRING (collector == (int)traversalGC ? "traversal" : (collector == (int)markSweepGC ? "markSweep" : (collector == (int)recyclerGC ? "recycler" : (collector == (int)balanced ? "balanced" : (collector == (int)markSweepTB ? "mark-sweep (thread-based)" : "copying")))))
#define ALLOCATOR_STRING (allocator == (int)basicAlloc ? "basic" : (allocator == (int)regionBased ? "regionBased" : (allocator == (int)threadBased ? "threadBased" : "nextFit")))
#define WRITEBARRIER_STRING (writebarrier == (int)recycler ? "recycler" : (writebarrier == (int)referenceCounting ? "referenceCounting" : "disabled"))
#define FINALGC_STRING (finalGC == 1 ? "enabled" : "disabled")
#define ZOMBIES_STRING (catchZombies == 1 ? "enabled" : "disabled")
#define TRAVERSALDEPTH_STRING (countTraversalDepth == 1 ? "enabled" : "disabled")
#define LOCKING_STRING (lockingStats == 1 ? "enabled" : "disabled")

//added by Tristan
#define MAX64BIT 0xFFFFFFFFFFFFFFFF	//18446744073709551615
#define MAX32BIT 0xFFFFFFFF			//4294967295
#define MAX16BIT 0xFFFF				//65535
#define MAX8BIT 0xFF				//255

#endif
