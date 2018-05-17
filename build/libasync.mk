# Build rules for the I/O library

ASYNC_SRC_DIR = $(SRC)/IO/Async

ASYNC_SOURCES = \
	$(SRC)/IO/Async/AsioThread.cpp \
	$(SRC)/IO/Async/GlobalAsioThread.cpp

ifeq ($(TARGET_IS_LINUX)$(TARGET_IS_ANDROID),yn)
ASYNC_SOURCES += \
	$(SRC)/IO/Async/SignalListener.cpp
endif

$(eval $(call link-library,async,ASYNC))
