TASKFILE_SOURCES = \
	$(SRC)/Task/PolylineDecoder.cpp \
	$(SRC)/Task/XCTrackTaskDecoder.cpp \
	$(SRC)/Task/XCTrackTaskFile.cpp \
	$(SRC)/Task/TaskFile.cpp \
	$(SRC)/Task/TaskFileXCSoar.cpp \
	$(SRC)/Task/TaskFileSeeYou.cpp \
	$(SRC)/Task/TaskFileIGC.cpp \
	$(SRC)/Task/Serialiser.cpp \
	$(SRC)/Task/Deserialiser.cpp \
	$(SRC)/Task/SaveFile.cpp \
	$(SRC)/Task/LoadFile.cpp

TASKFILE_DEPENDS = TASK XML JSON CUPFILE

$(eval $(call link-library,libtaskfile,TASKFILE))
