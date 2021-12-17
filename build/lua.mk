LUA = y

ifeq ($(LUA),y)

ifeq ($(USE_THIRDPARTY_LIBS),y)
LIBLUA_LDLIBS = -llua
LIBLUA_CPPFLAGS =
else
$(eval $(call pkg-config-library,LIBLUA,lua5.4))
endif

LUA_SOURCES = \
	$(SRC)/lua/Ptr.cpp \
	$(SRC)/lua/Error.cxx \
	$(SRC)/lua/Catch.cpp \
	$(SRC)/lua/Persistent.cpp \
	$(SRC)/lua/Background.cpp \
	$(SRC)/lua/Associate.cpp \
	$(SRC)/lua/RunFile.cxx \
	$(SRC)/lua/StartFile.cpp \
	$(SRC)/lua/Log.cpp \
	$(SRC)/lua/Http.cpp \
	$(SRC)/lua/Timer.cpp \
	$(SRC)/lua/Geo.cpp \
	$(SRC)/lua/Map.cpp \
	$(SRC)/lua/Blackboard.cpp \
	$(SRC)/lua/Airspace.cpp \
	$(SRC)/lua/Dialogs.cpp \
	$(SRC)/lua/Legacy.cpp \
	$(SRC)/lua/Full.cpp \
	$(SRC)/lua/Basic.cpp \
	$(SRC)/lua/Task.cpp \
	$(SRC)/lua/Settings.cpp \
	$(SRC)/lua/Wind.cpp \
	$(SRC)/lua/Logger.cpp \
	$(SRC)/lua/Tracking.cpp \
	$(SRC)/lua/Replay.cpp \
	$(SRC)/lua/InputEvent.cpp \

ifeq ($(TARGET),ANDROID)
LUA_SOURCES += $(SRC)/lua/Android.cpp
endif

LUA_CPPFLAGS_INTERNAL = $(LIBLUA_CPPFLAGS) $(SCREEN_CPPFLAGS) $(LIBHTTP_CPPFLAGS)
LUA_LDLIBS = $(LIBLUA_LDLIBS)

$(eval $(call link-library,liblua,LUA))

endif
