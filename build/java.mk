ANT = ant
JAVAC = javac
JAVAH = javah
JARSIGNER = jarsigner
KEYTOOL = keytool

ifneq ($(V),2)
ANT += -quiet
else
JARSIGNER += -verbose
JAVAC += -verbose
KEYTOOL += -v
endif

ifeq ($(DEBUG),y)
JAVAC += -g
endif

ifeq ($(WERROR),y)
JAVAC += -Werror
endif
