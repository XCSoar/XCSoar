ANT = ant
JAVAC = javac
KEYTOOL = keytool
JARSIGNER = jarsigner

ifneq ($(V),2)
ANT += -quiet
else
JAVAC += -verbose
KEYTOOL += -v
JARSIGNER += -verbose
endif

ifeq ($(DEBUG),y)
JAVAC += -g
endif

ifeq ($(WERROR),y)
JAVAC += -Werror
endif
