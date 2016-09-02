LUA = y

ifeq ($(LUA),y)

ifeq ($(USE_THIRDPARTY_LIBS),y)
LIBLUA_LDLIBS = -llua
LIBLUA_CPPFLAGS =
else
$(eval $(call pkg-config-library,LIBLUA,lua5.2))
endif

LUA_SOURCES = \
	$(SRC)/Lua/Ptr.cpp \
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
        $(SRC)/Lua/Wind.cpp \
        $(SRC)/Lua/Logger.cpp \
        $(SRC)/Lua/Tracking.cpp \
		$(SRC)/Lua/Replay.cpp \
	    $(SRC)/Lua/InputEvent.cpp \

LUA_CPPFLAGS_INTERNAL = $(LIBLUA_CPPFLAGS) $(SCREEN_CPPFLAGS)
LUA_LDLIBS = $(LIBLUA_LDLIBS)

$(eval $(call link-library,liblua,LUA))

endif
