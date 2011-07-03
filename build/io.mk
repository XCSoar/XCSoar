# Build rules for the I/O library

IO_SRC_DIR = $(SRC)/IO

IO_SOURCES = \
	$(IO_SRC_DIR)/FileCache.cpp \
	$(IO_SRC_DIR)/FileSource.cpp \
	$(IO_SRC_DIR)/ZipSource.cpp \
	$(IO_SRC_DIR)/LineSplitter.cpp \
	$(IO_SRC_DIR)/ConvertLineReader.cpp \
	$(IO_SRC_DIR)/FileLineReader.cpp \
	$(IO_SRC_DIR)/KeyValueFileReader.cpp \
	$(IO_SRC_DIR)/KeyValueFileWriter.cpp \
	$(IO_SRC_DIR)/ZipLineReader.cpp \
	$(IO_SRC_DIR)/CSVLine.cpp \
	$(IO_SRC_DIR)/BatchTextWriter.cpp \
	$(IO_SRC_DIR)/TextWriter.cpp

IO_OBJS = $(call SRC_TO_OBJ,$(IO_SOURCES))

IO_LIBS = $(TARGET_OUTPUT_DIR)/io.a

$(IO_LIBS): $(IO_OBJS)
	@$(NQ)echo "  AR      $@"
	$(Q)$(AR) $(ARFLAGS) $@ $^
