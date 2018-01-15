APPKIT ?= n

ifeq ($(APPKIT),y)
APPKIT_LDLIBS = -framework AppKit
APPKIT_CPPFLAGS += -DUSE_APPKIT
endif
