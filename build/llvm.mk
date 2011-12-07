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

ifeq ($(USE_CCACHE),y)
  # ccache will not use the optimisation of avoiding the 2nd call to the
  # pre-processor by compiling the pre-processed output that was used for
  # finding the hash in the case of a cache miss.
  CCACHE := export CCACHE_CPP2=yes && $(CCACHE)
endif

endif
