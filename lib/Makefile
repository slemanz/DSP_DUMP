# Compilation tools
CC := arm-none-eabi-gcc
ARMAR := arm-none-eabi-ar

# Optimized flags for the STM32F411 (Cortex-M4F).
# These must match the flags used by the applications that link against this
# archive, or the float ABI will not agree at link time.
# -ffunction-sections and -fdata-sections let the linker drop the DSP tables
# and functions the application never calls.
CFLAGS := -mcpu=cortex-m4 \
 -mfpu=fpv4-sp-d16 \
 -mfloat-abi=hard \
 -mthumb \
 -Os \
 -ffunction-sections \
 -fdata-sections \
 -Wsign-compare \
 -Wdouble-promotion \
 -ffast-math \
 -DNDEBUG \
 -DARM_MATH_CM4 \
 -D__FPU_PRESENT=1U \
 -Wall -Wextra -Werror

# Path to CMSIS
CMSIS_6 := CMSIS_6
CMSIS_DSP := CMSIS-DSP
CMSIS_CORE_INCLUDES := $(CMSIS_6)/CMSIS/Core/Include 

SRCS := $(CMSIS_DSP)/Source/BasicMathFunctions/BasicMathFunctions.c \
 $(CMSIS_DSP)/Source/CommonTables/CommonTables.c \
 $(CMSIS_DSP)/Source/InterpolationFunctions/InterpolationFunctions.c \
 $(CMSIS_DSP)/Source/BayesFunctions/BayesFunctions.c \
 $(CMSIS_DSP)/Source/MatrixFunctions/MatrixFunctions.c \
 $(CMSIS_DSP)/Source/ComplexMathFunctions/ComplexMathFunctions.c \
 $(CMSIS_DSP)/Source/QuaternionMathFunctions/QuaternionMathFunctions.c \
 $(CMSIS_DSP)/Source/ControllerFunctions/ControllerFunctions.c \
 $(CMSIS_DSP)/Source/SVMFunctions/SVMFunctions.c \
 $(CMSIS_DSP)/Source/DistanceFunctions/DistanceFunctions.c \
 $(CMSIS_DSP)/Source/StatisticsFunctions/StatisticsFunctions.c \
 $(CMSIS_DSP)/Source/FastMathFunctions/FastMathFunctions.c \
 $(CMSIS_DSP)/Source/SupportFunctions/SupportFunctions.c \
 $(CMSIS_DSP)/Source/FilteringFunctions/FilteringFunctions.c \
 $(CMSIS_DSP)/Source/TransformFunctions/TransformFunctions.c \
 $(CMSIS_DSP)/Source/WindowFunctions/WindowFunctions.c

# Includes
DSP_INCLUDES := $(CMSIS_DSP)/Include $(CMSIS_DSP)/PrivateInclude 
INC_FLAGS := $(addprefix -I,$(DSP_INCLUDES)) -I$(CMSIS_CORE_INCLUDES)
CFLAGS += $(INC_FLAGS)

# Output folder
BUILDDIR = build
OBJECTS := $(SRCS:%=$(BUILDDIR)/%.o)

all: $(BUILDDIR)/libCMSISDSP.a

# 's' builds the archive index, which makes linking faster and more reliable
$(BUILDDIR)/libCMSISDSP.a: $(OBJECTS)
	$(ARMAR) -rcs $@ $(OBJECTS)

# mkdir -p mirrors the CMSIS-DSP source tree under $(BUILDDIR)
$(BUILDDIR)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -rf $(BUILDDIR)

.PHONY: all clean