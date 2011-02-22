CLANG ?= n

ifeq ($(CLANG),y)
CXX = clang++
CC = clang
DEPFLAGS = -MD -MF $(DEPFILE) -MT $@

HOSTCC = $(CC)
HOSTCXX = $(CXX)

ifneq ($(DEBUG),y)
OPTIMIZE := -O4 -DNDEBUG -Wuninitialized
TARGET_LDFLAGS += -use-gold-plugin
AR += --plugin /usr/local/lib/libLLVMgold.so
endif

endif
