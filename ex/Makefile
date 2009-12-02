####### build verbosity

# Internal - Control verbosity
#  make V=0 - quiet
#  make V=1 - terse (default)
#  make V=2 - show commands
ifeq ($(V),2)
Q		:=
NQ		:=\#
else
Q		:=@
ifeq ($(V),0)
NQ		:=\#
else
NQ		:=
endif
endif

topdir = .
SRC := $(topdir)/src

include $(topdir)/build/warnings.mk

MAKEFLAGS	+=-r

PROFILE := 
# -pg
COVERAGE := 
# --coverage
OPTIMIZE := -O2 
DEBUG :=
# -g

INCLUDES := -I$(SRC)
CPPFLAGS := $(INCLUDES) 
CPPFLAGS += -DDO_PRINT  -DINSTRUMENT_TASK -DFIXED_MATH -DHAVE_BOOST

CFLAGS := $(DEBUG) $(OPTIMIZE) $(PROFILE) $(COVERAGE) $(C_WARNINGS)
CXXFLAGS := $(CFLAGS) -fno-exceptions $(CXX_WARNINGS)

#######

include $(topdir)/build/depend.mk
include $(topdir)/build/coverage.mk
include $(topdir)/build/doco.mk
include $(topdir)/build/task.mk
include $(topdir)/build/harness.mk
include $(topdir)/build/test.mk

####### sources

TARGETS = src/task.a src/harness.a $(TESTS) 

all: $(TARGETS)
	@echo "done"

include $(topdir)/build/rules.mk
include $(topdir)/build/clean.mk

