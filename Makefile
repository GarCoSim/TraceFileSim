CXXFLAGS = -Wall

ALLOCATORS = Allocators/Allocator.o Allocators/BasicAllocator.o Allocators/NextFitAllocator.o Allocators/RealAllocator.o
COLLECTORS = Collectors/Collector.o Collectors/MarkSweepCollector.o Collectors/TraversalCollector.o
MAIN = Main/CardTable.o Main/Object.o Main/ObjectContainer.o Main/MemoryManager.o Main/Simulator.o

all: traceFileSim

traceFileSim: $(ALLOCATORS) $(COLLECTORS) $(MAIN)

clean: ; rm -f traceFileSim Allocators/*.o Collectors/*.o Main/*.o
