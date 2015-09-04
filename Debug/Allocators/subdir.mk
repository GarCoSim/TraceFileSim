################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Allocators/Allocator.cpp \
../Allocators/BasicAllocator.cpp \
../Allocators/NextFitAllocator.cpp \
../Allocators/RealAllocator.cpp

OBJS += \
./Allocators/Allocator.o \
./Allocators/BasicAllocator.o \
./Allocators/NextFitAllocator.o \
./Allocators/RealAllocator.o

CPP_DEPS += \
./Allocators/Allocator.d \
./Allocators/BasicAllocator.d \
./Allocators/NextFitAllocator.d \
./Allocators/RealAllocator.d


# Each subdirectory must supply rules for building sources it contributes
Allocators/%.o: ../Allocators/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<" -pg
	@echo 'Finished building: $<'
	@echo ' '


