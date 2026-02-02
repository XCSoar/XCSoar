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
	$(SRC)/lua/Replay.cpp \
	$(SRC)/lua/InputEvent.cpp \

ifeq ($(TARGET),ANDROID)
LUA_SOURCES += $(SRC)/lua/Android.cpp
endif

ifeq ($(HAVE_HTTP),y)
LUA_SOURCES += $(SRC)/lua/Http.cpp
LUA_SOURCES += $(SRC)/lua/Tracking.cpp
endif

LUA_CPPFLAGS_INTERNAL = $(SCREEN_CPPFLAGS)

ifeq ($(HAVE_HTTP),y)
LUA_CPPFLAGS_INTERNAL += $(LIBHTTP_CPPFLAGS)
endif

LUA_DEPENDS = LIBLUA

ifeq ($(HAVE_HTTP),y)
ifeq ($(USE_THIRDPARTY_LIBS),y)
# libcurl is static in third-party builds, so avoid dllimport symbols
LUA_CPPFLAGS_INTERNAL += -DCURL_STATICLIB
LUA_DEPENDS += CURL
endif
endif

$(eval $(call link-library,liblua,LUA))

endif
