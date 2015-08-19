################################################################################
# Automatically-generated file. Do not edit!
################################################################################

CPPFLAGS = -pg
CXXFLAGS = -pg

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Main/MemoryManager.cpp \
../Main/Object.cpp \
../Main/ObjectContainer.cpp \
../Main/Simulator.cpp \
../Main/CardTable.cpp

OBJS += \
./Main/MemoryManager.o \
./Main/Object.o \
./Main/ObjectContainer.o \
./Main/Simulator.o \
./Main/CardTable.o

CPP_DEPS += \
./Main/MemoryManager.d \
./Main/Object.d \
./Main/ObjectContainer.d \
./Main/Simulator.d \
./Main/CardTable.d


# Each subdirectory must supply rules for building sources it contributes
Main/%.o: ../Main/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<" -pg
	@echo 'Finished building: $<'
	@echo ' '


