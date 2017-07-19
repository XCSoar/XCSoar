ANT = ant
JAVAC = javac
JAVAH = javah
JARSIGNER = jarsigner

ifneq ($(V),2)
ANT += -quiet
else
JARSIGNER += -verbose
JAVAC += -verbose
endif

ifeq ($(DEBUG),y)
JAVAC += -g
endif

ifeq ($(WERROR),y)
JAVAC += -Werror
endif
