ifeq ($(DEBUG),y)
OPTIMIZE := -O0 -ggdb

  ifneq ($(TARGET),UNIX)
    # WINE works best with stabs debug symbols
    OPTIMIZE += -gstabs
  endif
else
OPTIMIZE := -Os -DNDEBUG -Wuninitialized
endif

PROFILE :=
