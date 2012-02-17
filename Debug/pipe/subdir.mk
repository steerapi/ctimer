################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../pipe/pipe.c \
../pipe/pipe_util.c 

OBJS += \
./pipe/pipe.o \
./pipe/pipe_util.o 

C_DEPS += \
./pipe/pipe.d \
./pipe/pipe_util.d 


# Each subdirectory must supply rules for building sources it contributes
pipe/%.o: ../pipe/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -std=c99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


