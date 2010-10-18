USE_GCONF = $(call bool_not,$(call bool_or,$(HAVE_WIN32),$(PROFILE_MAP)))

ifeq ($(USE_GCONF),y)
  GCONF_CPPFLAGS += $(shell pkg-config --cflags gconf-2.0)
  GCONF_LDLIBS += $(shell pkg-config --libs gconf-2.0)
endif
