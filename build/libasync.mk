# Build rules for the I/O library

$(eval $(call pkg-config-library,CARES,libcares))

ASYNC_SRC_DIR = $(SRC)/io/Async

ASYNC_SOURCES = \
	$(SRC)/event/Loop.cxx \
	$(SRC)/event/Call.cxx \
	$(SRC)/event/DeferEvent.cxx \
	$(SRC)/event/InjectEvent.cxx \
	$(SRC)/event/SocketEvent.cxx \
	$(SRC)/event/SignalMonitor.cxx \
	$(SRC)/event/TimerWheel.cxx \
	$(SRC)/event/TimerList.cxx \
	$(SRC)/event/CoarseTimerEvent.cxx \
	$(SRC)/event/FineTimerEvent.cxx \
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

ASYNC_DEPENDS = CARES OS FMT

$(eval $(call link-library,async,ASYNC))
