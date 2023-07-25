TASKFILE_SOURCES = \
	$(SRC)/Task/TaskFile.cpp \
	$(SRC)/Task/TaskFileXCSoar.cpp \
	$(SRC)/Task/TaskFileSeeYou.cpp \
	$(SRC)/Task/TaskFileIGC.cpp \
	$(SRC)/Task/Serialiser.cpp \
	$(SRC)/Task/Deserialiser.cpp \
	$(SRC)/Task/SaveFile.cpp \
	$(SRC)/Task/LoadFile.cpp

TASKFILE_DEPENDS = TASK XML

$(eval $(call link-library,libtaskfile,TASKFILE))
