OPERATION_SOURCES := \
	$(SRC)/Operation/Operation.cpp \
	$(SRC)/Operation/PluggableOperationEnvironment.cpp \
	$(SRC)/Operation/ProxyOperationEnvironment.cpp \
	$(SRC)/Operation/NoCancelOperationEnvironment.cpp \
	$(SRC)/Operation/ThreadedOperationEnvironment.cpp

# This is necessary because ThreadedOperationEnvironment depends on
# class UI::DelayedNotify, which embeds a UI::Timer, which has a
# different implementation based on macro USE_POLL_EVENT
OPERATION_CPPFLAGS = $(POLL_EVENT_CPPFLAGS)

$(eval $(call link-library,liboperation,OPERATION))
