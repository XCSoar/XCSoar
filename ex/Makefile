
topdir = .
SRC := $(topdir)/src
TEST_SRC_DIR = Test

include $(topdir)/build/verbose.mk
include $(topdir)/build/targets.mk
include $(topdir)/build/debug.mk
include $(topdir)/build/warnings.mk
include $(topdir)/build/compile.mk
include $(topdir)/build/coverage.mk

MAKEFLAGS	+=-r

INCLUDES := $(TARGET_INCLUDES) -I$(SRC)
CPPFLAGS := $(INCLUDES) 
CPPFLAGS += -DFIXED_MATH 

ifeq ($(HAVE_WIN32),n)
CPPFLAGS += -DDO_PRINT 
CPPFLAGS += -DINSTRUMENT_TASK 
CPPFLAGS += -DHAVE_TAP
CPPFLAGS += -DHAVE_BOOST
endif

CFLAGS := $(OPTIMIZE) $(FLAGS_PROFILE) $(FLAGS_COVERAGE) $(C_WARNINGS)
CXXFLAGS := $(CFLAGS) -fno-exceptions $(CXX_WARNINGS)

#######

include $(topdir)/build/depend.mk
include $(topdir)/build/doco.mk
include $(topdir)/build/task.mk
include $(topdir)/build/harness.mk
include $(topdir)/build/test.mk

####### sources

OUTPUTS = $(SRC)/task-$(TARGET).a $(TEST_SRC_DIR)/harness-$(TARGET).a $(TESTS)

all: $(OUTPUTS)
	@echo "done"

include $(topdir)/build/rules.mk
include $(topdir)/build/clean.mk

