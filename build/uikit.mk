UIKIT ?= n

ifeq ($(UIKIT),y)
UIKIT_LDLIBS = -framework UiKit
UIKIT_CPPFLAGS += -DUSE_UIKIT
endif
