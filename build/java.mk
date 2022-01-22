ANT = ant
JAVAC = javac
KEYTOOL = keytool

ifneq ($(V),2)
ANT += -quiet
else
JAVAC += -verbose
KEYTOOL += -v
endif

ifeq ($(DEBUG),y)
JAVAC += -g
endif

ifeq ($(WERROR),y)
JAVAC += -Werror
endif
