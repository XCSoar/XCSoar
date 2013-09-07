DEBUG ?= y
DEBUG_GLIBCXX ?= $(DEBUG)

ifeq ($(DEBUG),y)
OPTIMIZE := -O0
OPTIMIZE += -funit-at-a-time
else
  ifeq ($(CLANG)$(LLVM)$(LTO),yny)
    OPTIMIZE := -O4
  else
    OPTIMIZE := -Os
  endif

  OPTIMIZE += -DNDEBUG
endif

ifeq ($(CLANG),y)
  OPTIMIZE += -g
else
ifeq ($(HAVE_WIN32),y)
  # WINE works best with stabs debug symbols
  OPTIMIZE += -gstabs
else
  OPTIMIZE += -g
endif
endif

# Enable fast floating point math.  XCSoar does not rely on strict
# IEEE/ISO semantics, for example it is not interested in "errno" or
# the difference between -0 and +0.  This allows using non-conforming
# vector units on some platforms, e.g. ARM NEON.
OPTIMIZE += -ffast-math

ifeq ($(CLANG)$(DEBUG),nn)
# Enable gcc auto-vectorisation on some architectures (e.g. ARM NEON).
# This requires -ffast-math, because some vector units don't conform
# stricly with IEEE/ISO (see above).
OPTIMIZE += -ftree-vectorize

OPTIMIZE += -funsafe-loop-optimizations
endif

ifeq ($(LTO),y)
OPTIMIZE += -flto -fwhole-program
endif

ifeq ($(LLVM),y)
# generate LLVM bitcode
OPTIMIZE += -emit-llvm
endif

ifeq ($(PROFILE),y)
FLAGS_PROFILE := -pg
else
FLAGS_PROFILE :=
endif
