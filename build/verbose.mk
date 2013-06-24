####### build verbosity

# Internal - Control verbosity
#  make V=0 - quiet
#  make V=1 - terse (default)
#  make V=2 - show commands
ifeq ($(V),2)
Q		:=
NQ		:=\#
else
Q		:=@
ifeq ($(V),0)
NQ		:=\#
else
NQ		:=
endif
endif
ifeq ($(HOST_IS_WIN32),y)
NQ		:=
endif
