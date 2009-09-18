CXXFLAGS += -Wall -Wextra
CXXFLAGS += -Wwrite-strings -Wcast-qual -Wpointer-arith -Wsign-compare
CXXFLAGS += -Wmissing-noreturn -Wundef

# disable some warnings, we're not ready for them yet
CXXFLAGS += -Wno-unused-parameter -Wno-format -Wno-reorder -Wno-missing-field-initializers -Wno-switch
CXXFLAGS += -Wno-non-virtual-dtor

# InputEvents_defaults.cpp should be fixed
CXXFLAGS += -Wno-char-subscripts

# FastMath.h does dirty aliasing tricks
CXXFLAGS += -Wno-strict-aliasing

# make warnings fatal (for perfectionists)
#CXXFLAGS += -Werror
#CXXFLAGS += -pedantic
#CXXFLAGS += -pedantic-errors

# -Wdisabled-optimization
# -Wunused -Wshadow -Wunreachable-code
