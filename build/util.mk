OBJ_SUFFIX = -$(TARGET).o

# Converts a list of source file names to *.o
SRC_TO_OBJ = $(patsubst %.cpp,%$(OBJ_SUFFIX),$(patsubst %.c,%$(OBJ_SUFFIX),$(1)))

%/dirstamp:
	@mkdir -p $(@D)
	@touch $@
