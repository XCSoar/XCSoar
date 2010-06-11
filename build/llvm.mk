CLANG ?= n

ifeq ($(CLANG),y)
CXX = clang++
CC = clang
DEPFLAGS =

HOSTCC = $(CC)
HOSTCXX = $(CXX)

ifneq ($(DEBUG),y)
OPTIMIZE := -O4 -DNDEBUG -Wuninitialized
endif

endif
