bin_PROGRAMS = volmix

volmix_SOURCES = volmix.c pulse_client.c pulse_client.h

volmix_CFLAGS = $(GTK_CFLAGS) $(PULSE_CFLAGS) $(GLIB_CFLAGS) -DDATADIR=\"$(datadir)\"
volmix_LDADD = $(GTK_LIBS) $(PULSE_LIBS) $(GLIB_LIBS)