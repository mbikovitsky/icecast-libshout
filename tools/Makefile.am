AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = shoutout
AM_CFLAGS = @XIPH_CFLAGS@ -I$(top_builddir)/include -Wall -Wextra

shoutout_SOURCES = shoutout.c
shoutout_LDADD = $(top_builddir)/src/libshout.la
