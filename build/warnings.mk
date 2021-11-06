WARNINGS = -Wall -Wextra
WARNINGS += -Wwrite-strings -Wcast-qual -Wpointer-arith -Wsign-compare
WARNINGS += -Wundef
WARNINGS += -Wmissing-declarations
WARNINGS += -Wredundant-decls

CXX_WARNINGS = $(WARNINGS)
CXX_WARNINGS += -Wmissing-noreturn
CXX_WARNINGS += -Wvla

# these warnings are not helpful
ifeq ($(CLANG),n)
CXX_WARNINGS += -Wno-format-truncation
endif

# disable some warnings, we're not ready for them yet
CXX_WARNINGS += -Wno-unused-parameter
CXX_WARNINGS += -Wno-missing-field-initializers
CXX_WARNINGS += -Wcast-align

# plain C warnings

C_WARNINGS = $(WARNINGS)
C_WARNINGS += -Wmissing-prototypes -Wstrict-prototypes
C_WARNINGS += -Wnested-externs

# make warnings fatal (for perfectionists)

WERROR ?= $(DEBUG)

ifeq ($(WERROR),y)
CXX_WARNINGS += -Werror
C_WARNINGS += -Werror
endif

#CXX_WARNINGS += -pedantic
#CXX_WARNINGS += -pedantic-errors

# -Wdisabled-optimization
# -Wunused -Wshadow -Wunreachable-code
# -Wfloat-equal
