CXXFLAGS = -Wall -g

ALLOCATORS = Allocators/Allocator.o Allocators/BasicAllocator.o Allocators/NextFitAllocator.o
COLLECTORS = Collectors/Collector.o Collectors/MarkSweepCollector.o Collectors/TraversalCollector.o Collectors/RecyclerCollector.o
MAIN = Main/Object.o Main/ObjectContainer.o Main/MemoryManager.o Main/Simulator.o
WRITEBARRIERS = WriteBarriers/WriteBarrier.o WriteBarriers/RecyclerWriteBarrier.o WriteBarriers/ReferenceCountingWriteBarrier.o

all: traceFileSim

traceFileSim: $(ALLOCATORS) $(COLLECTORS) $(MAIN) $(WRITEBARRIERS)

clean: ; rm -f traceFileSim Allocators/*.o Collectors/*.o Main/*.o WriteBarriers/*.o