# Build rules for the I/O library

IO_SRC_DIR = $(SRC)/IO

IO_SOURCES = \
	$(IO_SRC_DIR)/BufferedReader.cxx \
	$(IO_SRC_DIR)/FileReader.cxx \
	$(IO_SRC_DIR)/BufferedOutputStream.cxx \
	$(IO_SRC_DIR)/FileOutputStream.cxx \
	$(IO_SRC_DIR)/FileTransaction.cpp \
	$(IO_SRC_DIR)/FileCache.cpp \
	$(IO_SRC_DIR)/FileSource.cpp \
	$(IO_SRC_DIR)/ZipSource.cpp \
	$(IO_SRC_DIR)/InflateSource.cpp \
	$(IO_SRC_DIR)/LineSplitter.cpp \
	$(IO_SRC_DIR)/ConvertLineReader.cpp \
	$(IO_SRC_DIR)/FileLineReader.cpp \
	$(IO_SRC_DIR)/KeyValueFileReader.cpp \
	$(IO_SRC_DIR)/KeyValueFileWriter.cpp \
	$(IO_SRC_DIR)/ZipLineReader.cpp \
	$(IO_SRC_DIR)/InflateLineReader.cpp \
	$(IO_SRC_DIR)/CSVLine.cpp \
	$(IO_SRC_DIR)/TextWriter.cpp

IO_CPPFLAGS_INTERNAL = $(ZLIB_CPPFLAGS)

$(eval $(call link-library,io,IO))
