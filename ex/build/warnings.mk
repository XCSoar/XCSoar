
C_WARNINGS := \
	-Wall \
	-Wextra \
	-Wwrite-strings \
	-Wcast-qual \
	-Wpointer-arith \
	-Wsign-compare \
	-Wmissing-noreturn \
	-Wundef \
	-Wno-unused-parameter \
#	-Wno-missing-field-initializers \
#	-Wno-variadic-macros \
	-Wdisabled-optimization \
	-Wshadow 

# disable some warnings, we're not ready for them yet
#C_WARNINGS += -Wno-format -Wno-switch
# InputEvents_defaults.cpp should be fixed
#C_WARNINGS += -Wno-char-subscripts
# FastMath.h does dirty aliasing tricks
#C_WARNINGS += -Wno-strict-aliasing

CXX_WARNINGS := \
	 -Wno-non-virtual-dtor \
	 -Werror

