# Build rules for the OS specific utility/abstraction library

OS_SRC_DIR = $(SRC)/OS

OS_SOURCES := \
	$(OS_SRC_DIR)/Clock.cpp \
	$(OS_SRC_DIR)/FileDescriptor.cxx \
	$(OS_SRC_DIR)/FileMapping.cpp \
	$(OS_SRC_DIR)/FileUtil.cpp \
	$(OS_SRC_DIR)/RunFile.cpp \
	$(OS_SRC_DIR)/Path.cpp \
	$(OS_SRC_DIR)/PathName.cpp \
	$(OS_SRC_DIR)/Process.cpp \
	$(OS_SRC_DIR)/SystemLoad.cpp

ifeq ($(HAVE_POSIX),y)
OS_SOURCES += \
	$(OS_SRC_DIR)/EventPipe.cpp
endif

$(eval $(call link-library,libos,OS))

ifeq ($(HAVE_POSIX),n)
OS_LDLIBS += -lws2_32
endif
