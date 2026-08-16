CC=arm-none-eabi-gcc
TARGET=flash
APP ?= linear_homogeneity

MACH=-mcpu=cortex-m4
FLOAT=-mfloat-abi=hard -mfpu=fpv4-sp-d16

BUILD_DIR = Build
LIB_DIR = ../lib

PORT ?= /dev/ttyUSB0
BAUD ?= 115200

LINKER= -T linkers/STM32F411.ld
MAP_FILE= -Wl,-Map=$(BUILD_DIR)/$(TARGET).map

CMSIS_DSP_LIB = -L $(LIB_DIR)/build -l CMSISDSP

CFLAGS= -c $(MACH) -mthumb $(FLOAT) -std=gnu99 -Wall -O0 -g3 \
		--specs=nano.specs -ffunction-sections -fdata-sections

LDFLAGS = $(MACH) -mthumb $(FLOAT) --specs=nosys.specs $(LINKER) $(MAP_FILE) -g3 \
			-Wl,--gc-sections -static --specs=nano.specs -Wl,--start-group -lc -lm \
			-Wl,--end-group -u _printf_float -Wl,--print-memory-usage

LDFLAGS += -Wl,--no-warn-rwx-segments

OBJCOPY=arm-none-eabi-objcopy

###########################################
#				 INCLUDES
###########################################

INCLUDES+= -I app/Inc
INCLUDES+= -I drivers/Inc

INCLUDES+= -I $(LIB_DIR)/CMSIS-DSP/Include
INCLUDES+= -I $(LIB_DIR)/CMSIS_6/CMSIS/Core/Include


############################################
# 				SOURCE FILES
############################################

OBJS		+= $(BUILD_DIR)/$(APP).o
OBJS		+= $(BUILD_DIR)/config.o
OBJS		+= $(BUILD_DIR)/syscalls.o
OBJS		+= $(BUILD_DIR)/startup.o

OBJS		+= $(BUILD_DIR)/signals.o
OBJS		+= $(BUILD_DIR)/probe.o
OBJS		+= $(BUILD_DIR)/systems.o

DRIVERS		+= $(BUILD_DIR)/driver_clock.o
DRIVERS		+= $(BUILD_DIR)/driver_systick.o
DRIVERS		+= $(BUILD_DIR)/driver_gpio.o
DRIVERS		+= $(BUILD_DIR)/driver_uart.o
DRIVERS		+= $(BUILD_DIR)/driver_interrupt.o
DRIVERS		+= $(BUILD_DIR)/driver_fpu.o
DRIVERS		+= $(BUILD_DIR)/driver_adc.o


PREREQ = $(BUILD_DIR)

all: $(PREREQ) $(BUILD_DIR)/$(TARGET).elf

$(PREREQ):
	mkdir -p $(PREREQ)

$(BUILD_DIR)/%.o: app/Src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $(BUILD_DIR)/$(*).o app/Src/$(*).c

$(BUILD_DIR)/%.o: drivers/Src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $(BUILD_DIR)/$(*).o drivers/Src/$(*).c

$(BUILD_DIR)/%.o: linkers/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $(BUILD_DIR)/$(*).o linkers/$(*).c

# TARGET ELF
$(BUILD_DIR)/$(TARGET).elf: $(OBJS) $(DRIVERS)
	$(CC) $(LDFLAGS) -o $@ $^ $(CMSIS_DSP_LIB)
	$(OBJCOPY) -O binary $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin
	arm-none-eabi-size $(BUILD_DIR)/$(TARGET).elf

systems_tour:
	$(MAKE) all APP=systems_tour

linear_homogeneity:
	$(MAKE) all APP=linear_homogeneity

linear_additivity:
	$(MAKE) all APP=linear_additivity

linear_shift:
	$(MAKE) all APP=linear_shift

linear_superposition:
	$(MAKE) all APP=linear_superposition

linear_impulse:
	$(MAKE) all APP=linear_impulse

linear_step:
	$(MAKE) all APP=linear_step

load:
	jlink flash.jlink

monitor:
	picocom -b $(BAUD) $(PORT)

debug:
	ozone $(abspath workspaces/app.jdebug)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all systems_tour load monitor debug clean
