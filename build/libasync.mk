# Build rules for the I/O library

ASYNC_SRC_DIR = $(SRC)/IO/Async

ifeq ($(HAVE_POSIX),y)
ASYNC_SOURCES += \
	$(SRC)/IO/Async/IOThread.cpp \
	$(SRC)/IO/Async/GlobalIOThread.cpp
endif

$(eval $(call link-library,async,ASYNC))
