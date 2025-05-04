MKDIR = mkdir
NUL = /dev/null

ZIP = zip
UNZIP = unzip

ifneq ($(V),2)
ZIP += -q
UNZIP += -q
endif
