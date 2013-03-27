PORT_SRC_DIR = $(SRC)/Device/Port

PORT_SOURCES = \
	$(SRC)/Device/Port/Port.cpp \
	$(SRC)/Device/Port/BufferedPort.cpp \
	$(SRC)/Device/Port/SocketPort.cpp \
	$(SRC)/Device/Port/TCPPort.cpp \
	$(SRC)/Device/Port/K6BtPort.cpp \
	$(SRC)/Device/Port/NullPort.cpp

ifeq ($(HAVE_POSIX),y)
PORT_SOURCES += \
	$(SRC)/Device/Port/TTYEnumerator.cpp \
	$(SRC)/Device/Port/TTYPort.cpp
else
PORT_SOURCES += $(SRC)/Device/Port/SerialPort.cpp
endif

ifeq ($(HAVE_CE),y)
PORT_SOURCES += $(SRC)/Device/Port/Widcomm.cpp
endif

ifneq ($(DEBUG),n)
PORT_SOURCES += $(SRC)/Device/Port/DumpPort.cpp
endif

$(eval $(call link-library,port,PORT))

ifeq ($(TARGET),ANDROID)
# broken Android headers
$(call SRC_TO_OBJ,$(SRC)/Device/Port/SocketPort.cpp): CXXFLAGS += -Wno-cast-align
$(call SRC_TO_OBJ,$(SRC)/Device/Port/TCPPort.cpp): CXXFLAGS += -Wno-cast-align
endif

ifeq ($(HAVE_POSIX),n)
# broken mingw32 4.4 headers
$(call SRC_TO_OBJ,$(SRC)/Device/Port/SocketPort.cpp): CXXFLAGS += -Wno-sign-compare
$(call SRC_TO_OBJ,$(SRC)/Device/Port/TCPPort.cpp): CXXFLAGS += -Wno-sign-compare
endif
