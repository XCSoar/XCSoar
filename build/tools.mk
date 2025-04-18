MKDIR = mkdir
NUL = /dev/null

ZIP = zip

ifneq ($(V),2)
ZIP += -q
endif
