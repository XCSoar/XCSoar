# Build rules for the OS specific utility/abstraction library

OS_SRC_DIR = $(SRC)/OS

OS_SOURCES := \
	$(OS_SRC_DIR)/Clock.cpp \
	$(OS_SRC_DIR)/FileDescriptor.cpp \
	$(OS_SRC_DIR)/FileMapping.cpp \
	$(OS_SRC_DIR)/FileUtil.cpp \
	$(OS_SRC_DIR)/PathName.cpp \
	$(OS_SRC_DIR)/SystemLoad.cpp

ifeq ($(HAVE_CE),y)
OS_SOURCES += $(OS_SRC_DIR)/MemInfo.cpp
endif

$(eval $(call link-library,libos,OS))
