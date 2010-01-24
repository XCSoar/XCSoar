ifeq ($(TARGET),UNIX)
  GCONF_CPPFLAGS += $(shell pkg-config --cflags gconf-2.0)
  GCONF_LDLIBS += $(shell pkg-config --libs gconf-2.0)
endif
