################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/worker.cc 

CC_SRCS += \
../src/listenfd.cc \
../src/main.cc \
../src/process_event.cc \
../src/socket_util.cc \
../src/timers.cc 

OBJS += \
./src/listenfd.o \
./src/main.o \
./src/process_event.o \
./src/socket_util.o \
./src/timers.o \
./src/worker.o 

CC_DEPS += \
./src/listenfd.d \
./src/main.d \
./src/process_event.d \
./src/socket_util.d \
./src/timers.d 

CPP_DEPS += \
./src/worker.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/%.o: ../src/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


