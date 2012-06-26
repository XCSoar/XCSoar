WARNINGS = -Wall -Wextra
WARNINGS += -Wwrite-strings -Wcast-qual -Wpointer-arith -Wsign-compare
WARNINGS += -Wundef
WARNINGS += -Wmissing-declarations

ifneq ($(TARGET),ANDROID)
WARNINGS += -Wredundant-decls
endif

CXXFLAGS += $(WARNINGS)
CXXFLAGS += -Wmissing-noreturn

# disable some warnings, we're not ready for them yet
CXXFLAGS += -Wno-unused-parameter -Wno-non-virtual-dtor
CXXFLAGS += -Wno-missing-field-initializers 
CXXFLAGS += -Wcast-align

# plain C warnings

CFLAGS += $(WARNINGS)
CFLAGS += -Wmissing-prototypes -Wstrict-prototypes

ifneq ($(TARGET),ANDROID)
CFLAGS += -Wnested-externs
endif

# make warnings fatal (for perfectionists)

ifeq ($(WERROR),)
ifneq ($(TARGET),CYGWIN)
WERROR = $(DEBUG)
endif
endif

ifeq ($(WERROR),y)
CXXFLAGS += -Werror
CFLAGS += -Werror
endif

#CXXFLAGS += -pedantic
#CXXFLAGS += -pedantic-errors

# -Wdisabled-optimization
# -Wunused -Wshadow -Wunreachable-code
# -Wfloat-equal
