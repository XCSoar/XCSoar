DEBUG ?= y

ifeq ($(DEBUG),y)
OPTIMIZE := -O0
OPTIMIZE += -funit-at-a-time
else
OPTIMIZE := -O2 -DNDEBUG -Wuninitialized
endif

ifeq ($(TARGET),UNIX)
  OPTIMIZE += -ggdb
else
  # WINE works best with stabs debug symbols
  OPTIMIZE += -gstabs
endif

ifeq ($(PROFILE),y)
FLAGS_PROFILE := -pg
else
FLAGS_PROFILE :=
endif
