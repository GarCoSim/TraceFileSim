# Trace File Simulator


## Introduction

	The Trace File Simulator tool has been created to ease the analysis of
	existing memory management techniques, as well the prototyping of new memory
	management techniques. Specifically, the Trace File Simulation tool has
	been created to prototype and test new Memory Allocation and Garbage
	Collection policies.

	The simulator operates by reading a trace file (see *Trace files* below for
	more info) line by line and carrying out the associated memory operations
	internally. Parameters such as Heap Size, and Allocation and Collection
	policies can be adjusted via command line options (more on this in the *Usage*
	section) to test how changing these parameters affects overall Memory
	Management performance.

	During the simulation run, garbage collection and memory allocation statistics
	are captured and saved to a file ending in a ".log" suffix. The format of
	these log files will be covered in detail in the *Log File" section.

	The Trace File Simulator has been intentionally designed to allow for the easy
	integration of future collection and allocation policies. All collectors and
	allocators are essentially 'plug-ins' to the rest of the project. Allocators
	and collectors adhere to a common interface, so adding a new allocator or
	collector is often as easy as creating a new sub-class, which overrides the
	corresponding `collect()` or `allocate()` method. For more information about
	contributing to the project, please feel free to contact us.


## Build Instructions

	After cloning the project from this repository, it can be built by issuing the
	following commands:

	* cd Debug
	* make

	The executable `TraceFileSimulator` will be created upon successful compilation.

	For subsequent compilations (after making modifications to the code, for
	example) use the following command:

	* make clean && make


## Usage

	Invoking a simulation run can be done with the following command (assuming you
	are currently in the Debug directory):

	* ./traceFileSim <path/to/tracefile>

	Upon completion, a summary of the allocation and collection activity will be
	written to a log file. This file will be located in the same directory as the
	trace file specified for simulation, but will end in a ".log" suffix.

	The simulation will use default settings in the absence of any additional
	command line parameters. Settings can also be explicitly specified.

	To specify a particular allocation policy:

	* ./TraceFileSimulator <path/to/tracefile> --allocator <policyname>

	Similarly, to specify a particular collection policy:

	* ./TraceFileSimulator <path/to/tracefile> --collector <policyname>
	
	To specify the size (in bytes) of the heap used during simulation:
	
	* ./TraceFileSimulator <path/to/tracefile> --tracefile <size>


	To see a full listing of the accepted command line parameters, and their
	corresponding options, invoke the simulator with no arguments:

	* ./TraceFileSimulator


## Trace files

	A trace file summarizes the memory management activity that occurs during a
	program's execution. Each line of the trace file represents a single memory
	management operation carried out by a particular thread of execution. Memory
	management operations are defined at the granularity of objects. Some
	examples of memory management operations are:
	
	* allocating a new object to heap

	* adding an object to a thread's root set

	* updating an object reference field within an existing object

	* removing an object from a thread's root set

	* removing an object from heap

	Trace files can be created in several ways:
	
	* writing them by hand
		* For trivial examples, or to gain a better understanding of the trace file
		format, trace files can be manually created. For several examples see the
		trace files located in the Tests directory of this project. These simple
		trace files can be useful when attempting to verify the behavior of a new
		collector.

	* generating 'synthetic' trace files
		* A Trace File Generator is being created in a related project being
		conducted by our research team. This generator creates trace files which
		exhibit similar traits, as would be expected from a true program execution,
		but are completely random in nature. For more information see <link to the
		TraceFileGen project>

	* instrumenting the execution of an existing program
		*  The information required to create a trace file can be collected by
		modifying the execution environment of a program. Our research team has been
		involved with instrumenting IBM's J9 JVM, but similar information could be
		collected from other virtual machines.


## Log File

	Allocation and Collection statistics are collected during simulation and
	written to a log file. This log file will be located in the same directory as
	the trace file argument specified, and will be named similarly to the
	trace file, except that it will end in a ".log" suffix. The log file contains
	information such as:

	* allocation policy specified
	* collection policy specified
	* heap size specified
	* total execution time of simulation

	Additionally, the log file contains a table which summarizes the garbage
	collections which occurred throughout the simulation. Each line in this table
	corresponds to a collection triggered during simulation. Columns of this table
	provide the following information:

	* Line
		* the line number of the trace file that the simulator was processing when
		the collection was triggered.

	* GC Reason
		* the cause of the collection being triggered. Collections are generally
		triggered due to a failed called to `allocate()`, but certain collection
		policies could trigger a collection for other reasons.

	* Total GCs
		* the number of collections that have been triggered in the simulation so
		far.

	* Objects Freed
		* the number of objects identified as dead, and subsequently freed during
		this collection.

	* Live Objects
		* the remaining number of objects in the heap after the collection has
		finished.

	* Heap Used
		* the size (in bytes) of the heap still occupied by live objects after the
		collection has finished.

	* Free Heap
		* the size (in bytes) of the heap available for allocation after the
		collection has finished.

	* Generation
		* the generation that was targeted by this collection. This column only
		applies to generational collection policies, and will otherwise be set to 0. 

	* GC Time
		* the amount of time (in seconds) taken for the collection to complete.
