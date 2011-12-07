GETTEXT_LDLIBS =
GETTEXT_LDADD =

ifeq ($(TARGET_IS_DARWIN),y)
GETTEXT_LDADD := /opt/local/lib/libintl.a /opt/local/lib/libiconv.a
endif
