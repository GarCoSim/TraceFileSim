################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Allocators/Allocator.cpp \
../Allocators/BasicAllocator.cpp \
../Allocators/NextFitAllocator.cpp \
../Allocators/RealAllocator.cpp \
../Allocators/ThreadBasedAllocator.cpp \
../Allocators/RegionBasedAllocator.cpp 


OBJS += \
./Allocators/Allocator.o \
./Allocators/BasicAllocator.o \
./Allocators/NextFitAllocator.o \
./Allocators/RealAllocator.o \
./Allocators/ThreadBasedAllocator.o \
./Allocators/RegionBasedAllocator.o

CPP_DEPS += \
./Allocators/Allocator.d \
./Allocators/BasicAllocator.d \
./Allocators/NextFitAllocator.d \
./Allocators/RealAllocator.d \
./Allocators/ThreadBasedAllocator.d \
./Allocators/RegionBasedAllocator.d

# Each subdirectory must supply rules for building sources it contributes
Allocators/%.o: ../Allocators/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<" -pg
	@echo 'Finished building: $<'
	@echo ' '


