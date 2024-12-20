################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
LD_SRCS += \
../src/lscript.ld 

C_SRCS += \
../src/bsp.c \
../src/complex.c \
../src/fft.c \
../src/fonts.c \
../src/lab2a.c \
../src/lcd.c \
../src/lcd_test.c \
../src/main.c \
../src/note.c \
../src/qepn.c \
../src/qfn.c \
../src/qfn_init.c \
../src/stream_grabber.c \
../src/trig.c 

OBJS += \
./src/bsp.o \
./src/complex.o \
./src/fft.o \
./src/fonts.o \
./src/lab2a.o \
./src/lcd.o \
./src/lcd_test.o \
./src/main.o \
./src/note.o \
./src/qepn.o \
./src/qfn.o \
./src/qfn_init.o \
./src/stream_grabber.o \
./src/trig.o 

C_DEPS += \
./src/bsp.d \
./src/complex.d \
./src/fft.d \
./src/fonts.d \
./src/lab2a.d \
./src/lcd.d \
./src/lcd_test.d \
./src/main.d \
./src/note.d \
./src/qepn.d \
./src/qfn.d \
./src/qfn_init.d \
./src/stream_grabber.d \
./src/trig.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MicroBlaze gcc compiler'
	mb-gcc -Wall -O0 -g3 -c -fmessage-length=0 -MT"$@" -IC:/Users/sawye/workspace/lab3a_hw/export/lab3a_hw/sw/lab3a_hw/standalone_domain/bspinclude/include -mlittle-endian -mxl-barrel-shift -mxl-pattern-compare -mno-xl-soft-div -mcpu=v11.0 -mno-xl-soft-mul -mhard-float -mxl-float-convert -mxl-float-sqrt -Wl,--no-relax -ffunction-sections -fdata-sections -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


