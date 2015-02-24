################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Objects/Allocator.cpp \
../Objects/Collector.cpp \
../Objects/CopyingCollector.cpp \
../Objects/MarkSweepCollector.cpp \
../Objects/MemoryManager.cpp \
../Objects/Object.cpp \
../Objects/ObjectContainer.cpp \
../Objects/Simulator.cpp 

OBJS += \
./Objects/Allocator.o \
./Objects/Collector.o \
./Objects/CopyingCollector.o \
./Objects/MarkSweepCollector.o \
./Objects/MemoryManager.o \
./Objects/Object.o \
./Objects/ObjectContainer.o \
./Objects/Simulator.o 

CPP_DEPS += \
./Objects/Allocator.d \
./Objects/Collector.d \
./Objects/CopyingCollector.d \
./Objects/MarkSweepCollector.d \
./Objects/MemoryManager.d \
./Objects/Object.d \
./Objects/ObjectContainer.d \
./Objects/Simulator.d 


# Each subdirectory must supply rules for building sources it contributes
Objects/%.o: ../Objects/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


