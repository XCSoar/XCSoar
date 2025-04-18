DEBUG ?= y
DEBUG_GLIBCXX ?= n

ifeq ($(DEBUG),y)
  TARGET_CPPFLAGS += -DXCSOAR_TESTING
  OPTIMIZE := -Og
  ifeq ($(CLANG),n)
    OPTIMIZE += -funit-at-a-time
  endif
else
  OPTIMIZE := -Os
  OPTIMIZE += -DNDEBUG
endif

TARGET_OPTIMIZE := -g
HOST_OPTIMIZE := -g

# Enable fast floating point math.  XCSoar does not rely on strict
# IEEE/ISO semantics, for example it is not interested in "errno" or
# the difference between -0 and +0.  This allows using non-conforming
# vector units on some platforms, e.g. ARM NEON.
OPTIMIZE += -ffast-math

ifeq ($(CLANG)$(DEBUG),nn)
  # Enable gcc auto-vectorisation on some architectures (e.g. ARM
  # NEON).  This requires -ffast-math, because some vector units don't
  # conform stricly with IEEE/ISO (see above).
  OPTIMIZE += -ftree-vectorize

  OPTIMIZE += -funsafe-loop-optimizations
endif

ifeq ($(LTO),y)
  ifeq ($(CLANG),n)
    # Let GCC figure out the number of available CPU threads itself
    TARGET_OPTIMIZE += -flto=auto
    # Only compile GIMPLE bytecode into the objects, thus reduce compile time,
    # and reveal any not LTO capable component in the tool chain
    # Otherwise the machine code in fat ojects could be used, but you have no idea
    # that LTO was not effective
    TARGET_OPTIMIZE += -fno-fat-lto-objects
  else
    ifeq ($(THIN_LTO),y)
      TARGET_OPTIMIZE += -flto=thin
    else
      TARGET_OPTIMIZE += -flto
    endif
  endif
endif

OPTIMIZE_LDFLAGS = $(filter-out -emit-llvm,$(OPTIMIZE) $(TARGET_OPTIMIZE))
ifeq ($(CLANG)$(TARGET_IS_DARWIN)$(LTO),yny)
  # The Gold linker is known to work for LTO with LLVM Clang.  LLD
  # might be an option in the future, when it working reliably.
  USE_LD ?= gold
  # The -Os flag is incorrecly passed to the LLVM Gold plugin and LLD. -O3 works.
  OPTIMIZE_LDFLAGS := $(subst -Os,-O3,$(OPTIMIZE_LDFLAGS))
endif

ifeq ($(PROFILE),y)
  FLAGS_PROFILE := -pg
else
  FLAGS_PROFILE :=
endif

ifeq ($(SANITIZE),y)
  SANITIZE_FLAGS := -fsanitize=address
else ifeq ($(SANITIZE),n)
  SANITIZE_FLAGS :=
else
  SANITIZE_FLAGS := -fsanitize=$(SANITIZE)
endif
