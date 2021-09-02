OPERATION_SOURCES := \
	$(SRC)/Operation/Operation.cpp \
	$(SRC)/Operation/ProxyOperationEnvironment.cpp \
	$(SRC)/Operation/NoCancelOperationEnvironment.cpp \
	$(SRC)/Operation/ThreadedOperationEnvironment.cpp

$(eval $(call link-library,liboperation,OPERATION))
