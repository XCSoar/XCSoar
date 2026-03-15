SQLITE ?= y

ifeq ($(SQLITE),y)

SQLITE_LDLIBS = -lsqlite3

endif
