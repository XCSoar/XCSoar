# Build rules for the I/O library

IO_SRC_DIR = $(SRC)/IO

IO_SOURCES = \
	$(IO_SRC_DIR)/TextReader.cpp \
	$(IO_SRC_DIR)/TextWriter.cpp \
	$(IO_SRC_DIR)/ZipTextReader.cpp

IO_OBJS = $(call SRC_TO_OBJ,$(IO_SOURCES))

IO_LIBS = $(TARGET_OUTPUT_DIR)/io.a

$(IO_LIBS): $(IO_OBJS)
	@$(NQ)echo "  AR      $@"
	$(Q)$(AR) $(ARFLAGS) $@ $^
