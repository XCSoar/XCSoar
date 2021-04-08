DEBUG ?= y
DEBUG_GLIBCXX ?= n

ifeq ($(DEBUG),y)
  OPTIMIZE := -O0
  OPTIMIZE += -funit-at-a-time
else
  OPTIMIZE := -Os
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
  # Enable gcc auto-vectorisation on some architectures (e.g. ARM
  # NEON).  This requires -ffast-math, because some vector units don't
  # conform stricly with IEEE/ISO (see above).
  OPTIMIZE += -ftree-vectorize

  OPTIMIZE += -funsafe-loop-optimizations
endif

ifeq ($(LTO),y)
  ifeq ($(CLANG),n)
    # 8 LTO threads - that's an arbitrary value, but better than the
    # default
    OPTIMIZE += -flto=8
  else
    ifeq ($(THIN_LTO),y)
      OPTIMIZE += -flto=thin
    else
      OPTIMIZE += -flto
    endif
  endif
endif

ifeq ($(LLVM),y)
  # generate LLVM bitcode
  OPTIMIZE += -emit-llvm
endif

OPTIMIZE_LDFLAGS = $(filter-out -emit-llvm,$(OPTIMIZE))
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
else
  SANITIZE_FLAGS :=
endif
