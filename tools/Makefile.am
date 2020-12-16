AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = shout
AM_CFLAGS = @XIPH_CFLAGS@ -I$(top_builddir)/include -Wall -Wextra

man1_MANS = shout.1

shout_SOURCES = shout.c
shout_LDADD = $(top_builddir)/src/libshout.la
