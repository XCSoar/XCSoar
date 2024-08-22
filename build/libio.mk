# Build rules for the I/O library

IO_SRC_DIR = $(SRC)/io

IO_SOURCES = \
	$(SRC)/lib/zlib/Error.cxx \
	$(SRC)/lib/zlib/GunzipReader.cxx \
	$(IO_SRC_DIR)/CopyFile.cxx \
	$(IO_SRC_DIR)/Open.cxx \
	$(IO_SRC_DIR)/Reader.cxx \
	$(IO_SRC_DIR)/MemoryReader.cxx \
	$(IO_SRC_DIR)/BufferedReader.cxx \
	$(IO_SRC_DIR)/BufferedLineReader.cpp \
	$(IO_SRC_DIR)/BufferedCsvReader.cpp \
	$(IO_SRC_DIR)/FileDescriptor.cxx \
	$(IO_SRC_DIR)/FileMapping.cpp \
	$(IO_SRC_DIR)/FileReader.cxx \
	$(IO_SRC_DIR)/BufferedOutputStream.cxx \
	$(IO_SRC_DIR)/FileOutputStream.cxx \
	$(IO_SRC_DIR)/FileTransaction.cpp \
	$(IO_SRC_DIR)/FileCache.cpp \
	$(IO_SRC_DIR)/ZipArchive.cpp \
	$(IO_SRC_DIR)/ZipReader.cpp \
	$(IO_SRC_DIR)/StringConverter.cpp \
	$(IO_SRC_DIR)/FileLineReader.cpp \
	$(IO_SRC_DIR)/KeyValueFileReader.cpp \
	$(IO_SRC_DIR)/KeyValueFileWriter.cpp \
	$(IO_SRC_DIR)/CSVLine.cpp

IO_DEPENDS = OS ZLIB FMT UTIL FMT

$(eval $(call link-library,io,IO))

IO_CPPFLAGS += $(FMT_CPPFLAGS)
