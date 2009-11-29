ifeq ($(TARGET),UNIX)
  CPPFLAGS += $(shell pkg-config --cflags gconf-2.0)
  LDLIBS += $(shell pkg-config --libs gconf-2.0)
endif
