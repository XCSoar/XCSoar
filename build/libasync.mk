# Build rules for the I/O library

ASYNC_SRC_DIR = $(SRC)/IO/Async

ASYNC_SOURCES = \
	$(SRC)/IO/Async/AsioThread.cpp \
	$(SRC)/IO/Async/GlobalAsioThread.cpp

$(eval $(call link-library,async,ASYNC))
