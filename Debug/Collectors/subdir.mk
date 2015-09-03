################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Collectors/Collector.cpp \
../Collectors/MarkSweepCollector.cpp \
../Collectors/TraversalCollector.cpp

OBJS += \
./Collectors/Collector.o \
./Collectors/MarkSweepCollector.o \
./Collectors/TraversalCollector.o

CPP_DEPS += \
./Collectors/Collector.d \
./Collectors/MarkSweepCollector.d \
./Collectors/TraversalCollector.d

# Each subdirectory must supply rules for building sources it contributes
Collectors/%.o: ../Collectors/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<" -pg
	@echo 'Finished building: $<'
	@echo ' '


