SQLITE ?= y

ifeq ($(SQLITE),y)

$(eval $(call pkg-config-library,SQLITE,sqlite3))

endif
