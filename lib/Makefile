# Compilation tools
CC := arm-none-eabi-gcc
ARMAR := arm-none-eabi-ar

# Optimized flags for STM32G4 (Cortex-M4)
# Added -ffunction-sections and -fdata-sections to allow the linker 
# to remove unused DSP tables and functions
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

# Sources (kept your list)
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
# Fixed the object naming pattern to match the rule below
OBJECTS := $(SRCS:%=$(BUILDDIR)/%.o)

all: $(BUILDDIR)/libCMSISDSP.a

# Added 's' to rcs to create an index, making linking faster and more reliable
$(BUILDDIR)/libCMSISDSP.a: $(OBJECTS)
	$(ARMAR) -rcs $@ $(OBJECTS)

# Fixed rule to correctly handle subdirectories in CMSIS-DSP
$(BUILDDIR)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -rf $(BUILDDIR)/$(CMSIS_DSP)