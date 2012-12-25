# Build rules for the I/O library

IO_SRC_DIR = $(SRC)/IO

IO_SOURCES = \
	$(IO_SRC_DIR)/FileTransaction.cpp \
	$(IO_SRC_DIR)/FileCache.cpp \
	$(IO_SRC_DIR)/FileSource.cpp \
	$(IO_SRC_DIR)/ZipSource.cpp \
	$(IO_SRC_DIR)/LineSplitter.cpp \
	$(IO_SRC_DIR)/ConvertLineReader.cpp \
	$(IO_SRC_DIR)/FileLineReader.cpp \
	$(IO_SRC_DIR)/KeyValueFileReader.cpp \
	$(IO_SRC_DIR)/KeyValueFileWriter.cpp \
	$(IO_SRC_DIR)/ZipLineReader.cpp \
	$(IO_SRC_DIR)/TextFile.cpp \
	$(IO_SRC_DIR)/CSVLine.cpp \
	$(IO_SRC_DIR)/BatchTextWriter.cpp \
	$(IO_SRC_DIR)/BinaryWriter.cpp \
	$(IO_SRC_DIR)/TextWriter.cpp

$(eval $(call link-library,io,IO))
