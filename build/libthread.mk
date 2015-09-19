# Build rules for the multithreading library

THREAD_SRC_DIR = $(SRC)/Thread

THREAD_SOURCES = \
	$(THREAD_SRC_DIR)/Thread.cpp \
	$(THREAD_SRC_DIR)/SuspensibleThread.cpp \
	$(THREAD_SRC_DIR)/RecursivelySuspensibleThread.cpp \
	$(THREAD_SRC_DIR)/WorkerThread.cpp \
	$(THREAD_SRC_DIR)/StandbyThread.cpp \
	$(THREAD_SRC_DIR)/Debug.cpp

# this is needed to compile Notify.cpp, which depends on the screen
# library's event queue
THREAD_CPPFLAGS_INTERNAL = $(SCREEN_CPPFLAGS)

$(eval $(call link-library,libthread,THREAD))
