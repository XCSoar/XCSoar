EVENT_SOURCES = \
	$(SRC)/Event/FunctionalTimer.cpp

EVENT_CPPFLAGS = $(SDL_CPPFLAGS) $(GDI_CPPFLAGS) $(EGL_CPPFLAGS)

$(eval $(call link-library,libevent,EVENT))
