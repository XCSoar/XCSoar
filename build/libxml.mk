XML_SOURCES = \
	$(SRC)/XML/Node.cpp \
	$(SRC)/XML/Parser.cpp \
	$(SRC)/XML/Writer.cpp \
	$(SRC)/XML/DataNode.cpp \
	$(SRC)/XML/DataNodeXML.cpp

XML_DEPENDS = IO TIME UTIL

$(eval $(call link-library,libxml,XML))
