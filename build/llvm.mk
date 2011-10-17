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
ARFLAGS += --plugin /usr/local/lib/LLVMgold.so
endif

endif
