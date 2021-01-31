# Build rules for the I/O library

$(eval $(call pkg-config-library,CARES,libcares))

ASYNC_SRC_DIR = $(SRC)/io/Async

ASYNC_SOURCES = \
	$(SRC)/event/Loop.cxx \
	$(SRC)/event/Call.cxx \
	$(SRC)/event/DeferEvent.cxx \
	$(SRC)/event/IdleEvent.cxx \
	$(SRC)/event/InjectEvent.cxx \
	$(SRC)/event/SocketEvent.cxx \
	$(SRC)/event/SignalMonitor.cxx \
	$(SRC)/event/TimerEvent.cxx \
	$(SRC)/event/net/ConnectSocket.cxx \
	$(SRC)/event/net/cares/Error.cxx \
	$(SRC)/event/net/cares/Init.cxx \
	$(SRC)/event/net/cares/Channel.cxx \
	$(SRC)/event/net/cares/SimpleResolver.cxx \
	$(SRC)/io/async/AsioThread.cpp \
	$(SRC)/io/async/GlobalAsioThread.cpp

ifeq ($(HAVE_WIN32),y)
ASYNC_SOURCES += \
	$(SRC)/event/WinSelectBackend.cxx
else
ifeq ($(TARGET_IS_LINUX),n)
ASYNC_SOURCES += \
	$(SRC)/event/PollBackend.cxx
endif
endif

ASYNC_CPPFLAGS_INTERNAL = $(CARES_CPPFLAGS)
ASYNC_LDLIBS = $(CARES_LDLIBS)

$(eval $(call link-library,async,ASYNC))
