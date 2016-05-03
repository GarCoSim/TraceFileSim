CXXFLAGS = -Wall -g

ALLOCATORS = Allocators/Allocator.o Allocators/BasicAllocator.o Allocators/NextFitAllocator.o Allocators/RealAllocator.o
COLLECTORS = Collectors/Collector.o Collectors/MarkSweepCollector.o Collectors/TraversalCollector.o
MAIN = Main/CardTable.o Main/Object.o Main/ObjectContainer.o Main/MemoryManager.o Main/Simulator.o
WRITEBARRIERS = WriteBarriers/WriteBarrier.o WriteBarriers/RecyclerWriteBarrier.o WriteBarriers/ZombieRecyclerWriteBarrier.o

all: traceFileSim

traceFileSim: $(ALLOCATORS) $(COLLECTORS) $(MAIN) $(WRITEBARRIERS)

clean: ; rm -f traceFileSim Allocators/*.o Collectors/*.o Main/*.o WriteBarriers/*.o