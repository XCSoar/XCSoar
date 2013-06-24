ANT = ant
JAVAH = javah
JARSIGNER = jarsigner

ifneq ($(V),2)
ANT += -quiet
else
JARSIGNER += -verbose
endif
