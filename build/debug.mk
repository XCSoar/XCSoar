ifeq ($(DEBUG),y)
OPTIMIZE := -O0 -ggdb
OPTIMIZE += -funit-at-a-time

  ifneq ($(TARGET),UNIX)
    # WINE works best with stabs debug symbols
    OPTIMIZE += -gstabs
  endif
else
OPTIMIZE := -O2 -DNDEBUG -Wuninitialized
endif

ifeq ($(PROFILE),y)
FLAGS_PROFILE := -pg
else
FLAGS_PROFILE :=
endif
