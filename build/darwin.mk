ifeq ($(TARGET_IS_DARWIN),y)

TARGET_LDLIBS += -framework Foundation

TARGET_CPPFLAGS += -isystem $(DARWIN_LIBS)/include
TARGET_CXXFLAGS += -ObjC++ -fobjc-arc

ifneq ($(DARWIN_SDK),)
TARGET_ARCH += -isysroot $(DARWIN_SDK)
endif

endif

