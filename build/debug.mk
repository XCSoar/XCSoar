ifeq ($(DEBUG),y)
OPTIMIZE := -O0 -ggdb
else
OPTIMIZE := -O2 -DNDEBUG -Wuninitialized
endif

PROFILE :=
