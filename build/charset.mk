CHARSET = utf-8

ifneq ($(CLANG),y)
CXX_FEATURES += -finput-charset=$(CHARSET)
C_FEATURES += -finput-charset=$(CHARSET)
endif
