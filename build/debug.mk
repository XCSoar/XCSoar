ifeq ($(DEBUG),y)
OPTIMIZE := -O0 -ggdb

  ifneq ($(TARGET),UNIX)
    # WINE works best with stabs debug symbols
    OPTIMIZE += -gstabs
  endif
else
OPTIMIZE := -O2 -DNDEBUG -Wuninitialized
endif

PROFILE :=
