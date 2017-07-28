CXXFLAGS = -Wall -g

ALLOCATORS = Allocators/Allocator.o Allocators/BasicAllocator.o Allocators/NextFitAllocator.o Allocators/RegionBasedAllocator.o
COLLECTORS = Collectors/Collector.o Collectors/MarkSweepCollector.o Collectors/TraversalCollector.o Collectors/RecyclerCollector.o Collectors/BalancedCollector.o
MAIN = Main/Array.o Main/Optional.o Main/CardTable.o Main/Object.o Main/ObjectContainer.o Main/MemoryManager.o Main/Simulator.o Main/Region.o
WRITEBARRIERS = WriteBarriers/WriteBarrier.o WriteBarriers/RecyclerWriteBarrier.o WriteBarriers/ReferenceCountingWriteBarrier.o

all: traceFileSim

traceFileSim: $(ALLOCATORS) $(COLLECTORS) $(MAIN) $(WRITEBARRIERS)

clean: ; rm -f traceFileSim Allocators/*.o Collectors/*.o Main/*.o WriteBarriers/*.o

clear_objects: ; rm -f Allocators/*.o Collectors/*.o Main/*.o WriteBarriers/*.o
