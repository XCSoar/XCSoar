# Lua scripting is off by default for now, until we have a
# cross-platform build script for the Lua interpreter.
LUA = n

ifeq ($(LUA),y)

$(eval $(call pkg-config-library,LUA,lua5.2))

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
	$(SRC)/Lua/Blackboard.cpp \
	$(SRC)/Lua/Dialogs.cpp \
	$(SRC)/Lua/Legacy.cpp \
	$(SRC)/Lua/Full.cpp \
	$(SRC)/Lua/Basic.cpp

$(eval $(call link-library,liblua,LUA))

LUA_CPPFLAGS += -DUSE_LUA

endif
