# Build rules for the generic math library

MATH_SRC_DIR = $(SRC)/Math
INCLUDES += -I$(MATH_SRC_DIR)

MATH_SOURCES = \
	$(MATH_SRC_DIR)/FastRotation.cpp \
	$(MATH_SRC_DIR)/leastsqs.cpp \
	$(MATH_SRC_DIR)/LowPassFilter.cpp

MATH_OBJS = $(call SRC_TO_OBJ,$(MATH_SOURCES))
MATH_LIBS = $(TARGET_OUTPUT_DIR)/math.a

$(MATH_LIBS): $(MATH_OBJS)
	@$(NQ)echo "  AR      $@"
	$(Q)$(AR) $(ARFLAGS) $@ $^
