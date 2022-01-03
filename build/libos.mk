# Build rules for the OS specific utility/abstraction library

OS_SRC_DIR = $(SRC)/system

OS_SOURCES := \
	$(OS_SRC_DIR)/EventPipe.cxx \
	$(OS_SRC_DIR)/FileMapping.cpp \
	$(OS_SRC_DIR)/FileUtil.cpp \
	$(OS_SRC_DIR)/RunFile.cpp \
	$(OS_SRC_DIR)/Path.cpp \
	$(OS_SRC_DIR)/PathName.cpp \
	$(OS_SRC_DIR)/Process.cpp \
	$(OS_SRC_DIR)/SystemLoad.cpp

ifeq ($(TARGET_IS_LINUX),y)
OS_SOURCES += \
	$(OS_SRC_DIR)/EventFD.cxx
endif

ifeq ($(TARGET_IS_LINUX),y)
OS_SOURCES += \
	$(OS_SRC_DIR)/EpollFD.cxx \
	$(OS_SRC_DIR)/EventFD.cxx \
	$(OS_SRC_DIR)/SignalFD.cxx
endif

$(eval $(call link-library,libos,OS))

ifeq ($(HAVE_POSIX),n)
OS_LDLIBS += -lws2_32
endif
