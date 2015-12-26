LUA = y

ifeq ($(LUA),y)

ifeq ($(USE_THIRDARTY_LIBS),y)
LUA_LDLIBS = -llua
LUA_CPPFLAGS =
else
$(eval $(call pkg-config-library,LUA,lua5.2))
endif

LUA_SOURCES = \
	$(SRC)/Lua/Error.cpp \
	$(SRC)/Lua/Catch.cpp \
	$(SRC)/Lua/Persistent.cpp \
	$(SRC)/Lua/Background.cpp \
	$(SRC)/Lua/Associate.cpp \
	$(SRC)/Lua/RunFile.cpp \
	$(SRC)/Lua/StartFile.cpp \
	$(SRC)/Lua/Log.cpp \
	$(SRC)/Lua/Timer.cpp \
	$(SRC)/Lua/Geo.cpp \
	$(SRC)/Lua/Map.cpp \
	$(SRC)/Lua/Blackboard.cpp \
	$(SRC)/Lua/Airspace.cpp \
	$(SRC)/Lua/Dialogs.cpp \
	$(SRC)/Lua/Legacy.cpp \
	$(SRC)/Lua/Full.cpp \
	$(SRC)/Lua/Basic.cpp \
        $(SRC)/Lua/Task.cpp \
        $(SRC)/Lua/Settings.cpp \
        $(SRC)/Lua/Wind.cpp

LUA_CPPFLAGS_INTERNAL = $(SCREEN_CPPFLAGS)

$(eval $(call link-library,liblua,LUA))

LUA_CPPFLAGS += -DUSE_LUA

endif
