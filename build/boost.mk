ifeq ($(HAVE_WIN32)$(findstring $(TARGET),ANDROID),n)
CPPFLAGS += -DHAVE_BOOST
endif
