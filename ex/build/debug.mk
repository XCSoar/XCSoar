ifeq ($(DEBUG),y)
OPTIMIZE := -O0 -ggdb -funit-at-a-time
  ifneq ($(TARGET),UNIX)
    # WINE works best with stabs debug symbols
    OPTIMIZE += -gstabs
  endif
else
OPTIMIZE := -O2 -DNDEBUG -Wuninitialized
endif

PROFILE :=
# -pg

COVERAGE := 
# --coverage
