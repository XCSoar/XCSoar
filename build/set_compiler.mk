######## tools


# Define the compiler commands early here. It may be required
# in pkgconfig.mk.
# The remainder of compile and link commands and options
# are defined in build/compile.mk.

EXE := $(findstring .exe,$(MAKE))

ifneq ($(ANALYZER),y)
  ifeq ($(CLANG),y)
    CXX = $(LLVM_PREFIX)clang++$(LLVM_SUFFIX)$(EXE)
    CC = $(LLVM_PREFIX)clang$(LLVM_SUFFIX)$(EXE)
  else
    CXX = $(TCPREFIX)g++$(TCSUFFIX)$(EXE)
    CC = $(TCPREFIX)gcc$(TCSUFFIX)$(EXE)
  endif
endif
