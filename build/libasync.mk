# Build rules for the I/O library

ASYNC_SRC_DIR = $(SRC)/io/Async

ASYNC_SOURCES = \
	$(SRC)/io/async/AsioThread.cpp \
	$(SRC)/io/async/GlobalAsioThread.cpp

ifeq ($(TARGET_IS_LINUX)$(TARGET_IS_ANDROID),yn)
ASYNC_SOURCES += \
	$(SRC)/io/async/SignalListener.cpp
endif

$(eval $(call link-library,async,ASYNC))

ASYNC_LDADD += $(THREAD_LDADD)
