topdir = .
SRC := $(topdir)/src

include $(topdir)/build/verbose.mk
include $(topdir)/build/targets.mk
include $(topdir)/build/debug.mk
include $(topdir)/build/warnings.mk
include $(topdir)/build/compile.mk

MAKEFLAGS	+=-r

INCLUDES := $(TARGET_INCLUDES) -I$(SRC)
CPPFLAGS := $(INCLUDES) 
CPPFLAGS += -DFIXED_MATH 

ifeq ($(HAVE_WIN32),n)
ifeq ($(DEBUG),y)
CPPFLAGS += -DDO_PRINT 
CPPFLAGS += -DINSTRUMENT_TASK 
endif
#CPPFLAGS += -DHAVE_TAP
CPPFLAGS += -DHAVE_BOOST
endif

CFLAGS := $(OPTIMIZE) $(PROFILE) $(COVERAGE) $(C_WARNINGS)
CXXFLAGS := $(CFLAGS) -fno-exceptions $(CXX_WARNINGS)

#######

include $(topdir)/build/depend.mk
include $(topdir)/build/coverage.mk
include $(topdir)/build/doco.mk
include $(topdir)/build/task.mk
include $(topdir)/build/harness.mk
include $(topdir)/build/test.mk

####### sources

OUTPUTS = src/task.a Test/harness.a $(TESTS) 

all: $(OUTPUTS)
	@echo "done"

include $(topdir)/build/rules.mk
include $(topdir)/build/clean.mk

