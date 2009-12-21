WARNINGS = -Wall -Wextra
WARNINGS += -Wwrite-strings -Wcast-qual -Wpointer-arith -Wsign-compare
WARNINGS += -Wundef

CXXFLAGS += $(WARNINGS)
CXXFLAGS += -Wmissing-noreturn

# disable some warnings, we're not ready for them yet
CXXFLAGS += -Wno-unused-parameter -Wno-format -Wno-reorder -Wno-switch -Wno-non-virtual-dtor

ifneq ($(WINHOST),y)
	CXXFLAGS += -Wno-missing-field-initializers 
endif

# FastMath.h does dirty aliasing tricks
CXXFLAGS += -Wno-strict-aliasing

# plain C warnings

CFLAGS += $(WARNINGS)

# make warnings fatal (for perfectionists)

ifeq ($(WERROR),)
WERROR = $(DEBUG)
endif

ifeq ($(WERROR),y)
CXXFLAGS += -Werror
CFLAGS += -Werror
endif

#CXXFLAGS += -pedantic
#CXXFLAGS += -pedantic-errors

# -Wdisabled-optimization
# -Wunused -Wshadow -Wunreachable-code
